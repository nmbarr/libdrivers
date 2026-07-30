# Collapsing HTS221 ReadCalibration to a Burst Read

A refactor note on `HTS221_ReadCalibration()`. The current implementation feels
clunky, and it's worth pinning down *why* — there are two different problems
tangled together, plus a latent bug that the cleanup removes.

## Problem 1: repeated read-check boilerplate

Every calibration field costs the same four-line ritual:

```c
status = HTS221_ReadReg(pHandle, HTS221_REG_X, &byte, 1);
if (status != LIBDRIVERS_OK) {
    return status;
}
```

This appears ~10 times. That's the surface-level noise. But the reason there
are ten of them is the real problem: the function does ten separate bus
transactions.

## Problem 2: ignoring a hardware feature the driver already has

`HTS221_ReadReg` sets the auto-increment bit (bit 7) so a multi-byte read walks
consecutive registers. And the calibration map is **contiguous, 0x30 through
0x3F** — sixteen bytes, back to back:

```
0x30  H0_RH_X2          0x38  (reserved)
0x31  H1_RH_X2          0x39  (reserved)
0x32  T0_DEGC_X8        0x3A  H1_T0_OUT_L
0x33  T1_DEGC_X8        0x3B  H1_T0_OUT_H
0x34  (reserved)        0x3C  T0_OUT_L
0x35  T0_T1_MSB         0x3D  T0_OUT_H
0x36  H0_T0_OUT_L       0x3E  T1_OUT_L
0x37  H0_T0_OUT_H       0x3F  T1_OUT_H
```

So the whole function body can be:

1. **One** burst read of 16 bytes starting at `0x30` into `uint8_t cal[16]`.
2. Unpack every field from that buffer **in memory** — no bus, no error checks,
   no status variable.

That collapses 10 transactions + 10 error checks into 1 transaction + 1 error
check. On an I²C bus each transaction carries real start/stop/address overhead,
so this is faster *and* shorter.

## The latent bug the cleanup removes

The current code reads `HTS221_REG_T0_T1_MSB` (0x35) **twice** — once for T0's
MSB bits, once for T1's — because both fields are packed into that single
register. Reading one register twice to extract two different bit-fields is
exactly the kind of thing that drifts out of sync during later edits. With a
burst read it becomes `cal[0x35 - 0x30]`: read once, masked two ways.

## Sketch of the unpack

```c
uint8_t cal[16];
Libdrivers_Status_t status = HTS221_ReadReg(pHandle, HTS221_REG_H0_RH_X2, cal, sizeof(cal));
if (status != LIBDRIVERS_OK) {
    return status;
}

// Everything below is pure memory math — indices are (reg - 0x30):
pHandle->H0_rH_x2 = cal[HTS221_REG_H0_RH_X2 - HTS221_REG_H0_RH_X2]; // cal[0]
uint8_t msb = cal[HTS221_REG_T0_T1_MSB - HTS221_REG_H0_RH_X2];      // cal[5], read ONCE
pHandle->T0_degC_x8 = ((msb & HTS221_T0_MSB_MASK) << 8) | cal[2];
pHandle->T1_degC_x8 = (((msb & HTS221_T1_MSB_MASK) >> HTS221_T1_MSB_SHIFT) << 8) | cal[3];
// ...int16 pairs the same way: (cal[hi] << 8) | cal[lo]
```

## Open judgment calls

- **The index expressions.** `cal[HTS221_REG_T0_DEGC_X8 - HTS221_REG_H0_RH_X2]`
  is self-documenting but noisy. A `#define HTS221_CAL_BASE 0x30` plus an inline
  helper/macro could clean it up — or a raw `cal[2]` with a comment may read
  more clearly. Judgment call.
- **Whether to keep single-byte `ReadReg` for calibration at all.** Once it's a
  burst, the LSB/MSB temporaries all disappear.
