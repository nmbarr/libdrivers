#ifndef LIBDRIVERS_TEST_HARNESS_H
#define LIBDRIVERS_TEST_HARNESS_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @file test_harness.h
 * @brief Minimal assertion + runner harness for the libdrivers unit tests.
 *
 * Deliberately dependency-free, for the same reason the drivers are: the test
 * build should need nothing but a C compiler. Each suite is one executable
 * that CTest runs on its own, so a failure names the driver it came from.
 *
 * A suite looks like:
 * @code
 *   TEST(thing_does_the_thing) {
 *       ASSERT_STATUS_EQ(LIBDRIVERS_OK, Driver_Thing(&handle));
 *   }
 *
 *   static const LibdriversTest tests[] = {
 *       LIBDRIVERS_TEST(thing_does_the_thing),
 *   };
 *   LIBDRIVERS_TEST_MAIN("driver", tests)
 * @endcode
 */

// Set by a failing assertion in the running test; checked by the runner.
// A test that fails an assertion returns immediately, so this never has to
// accumulate more than one failure per test.
static int libdrivers_test_failed;

/** @brief Define a test case. The body runs with the harness's failure flag clear. */
#define TEST(name) static void name(void)

// Every assertion below reports and bails out of the current test on failure:
// once a fact about the driver is wrong, later assertions in the same test are
// reading undefined state and their output is noise.
#define LIBDRIVERS_FAIL(fmt, ...)                                                                  \
    do {                                                                                           \
        printf("    FAIL %s:%d: " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);                      \
        libdrivers_test_failed = 1;                                                                \
        return;                                                                                    \
    } while (0)

/** @brief Assert a condition holds. */
#define ASSERT_TRUE(cond)                                                                          \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            LIBDRIVERS_FAIL("expected true: %s", #cond);                                           \
        }                                                                                          \
    } while (0)

/** @brief Assert two integers are equal (printed in decimal and hex on failure). */
#define ASSERT_INT_EQ(expected, actual)                                                            \
    do {                                                                                           \
        long long libdrivers_e = (long long)(expected);                                            \
        long long libdrivers_a = (long long)(actual);                                              \
        if (libdrivers_e != libdrivers_a) {                                                        \
            LIBDRIVERS_FAIL("%s: expected %lld (0x%llX), got %lld (0x%llX)", #actual,              \
                            libdrivers_e, (unsigned long long)libdrivers_e, libdrivers_a,          \
                            (unsigned long long)libdrivers_a);                                     \
        }                                                                                          \
    } while (0)

/** @brief Assert a Libdrivers_Status_t equals an expected value, by name. */
#define ASSERT_STATUS_EQ(expected, actual)                                                         \
    do {                                                                                           \
        Libdrivers_Status_t libdrivers_e = (expected);                                             \
        Libdrivers_Status_t libdrivers_a = (actual);                                               \
        if (libdrivers_e != libdrivers_a) {                                                        \
            LIBDRIVERS_FAIL("%s: expected %s, got %s", #actual,                                    \
                            libdrivers_test_status_name(libdrivers_e),                             \
                            libdrivers_test_status_name(libdrivers_a));                            \
        }                                                                                          \
    } while (0)

/**
 * @brief Assert two floats agree to within @p tol.
 *
 * Exact comparison is right for values the driver can produce exactly (a
 * DS18B20 count is a multiple of 1/16, which binary float holds exactly), but
 * most conversions divide by a non-power-of-two, so a tolerance is the honest
 * check. Pick one tight enough that a wrong scale factor cannot slip through.
 */
#define ASSERT_FLOAT_NEAR(expected, actual, tol)                                                   \
    do {                                                                                           \
        double libdrivers_e = (double)(expected);                                                  \
        double libdrivers_a = (double)(actual);                                                    \
        double libdrivers_d = libdrivers_e - libdrivers_a;                                         \
        if (libdrivers_d < 0) {                                                                    \
            libdrivers_d = -libdrivers_d;                                                          \
        }                                                                                          \
        if (!(libdrivers_d <= (double)(tol))) {                                                    \
            LIBDRIVERS_FAIL("%s: expected %g +/- %g, got %g", #actual, libdrivers_e,               \
                            (double)(tol), libdrivers_a);                                          \
        }                                                                                          \
    } while (0)

/** @brief Assert two byte buffers hold the same @p len bytes. */
#define ASSERT_BYTES_EQ(expected, actual, len)                                                     \
    do {                                                                                           \
        const uint8_t *libdrivers_e = (const uint8_t *)(expected);                                 \
        const uint8_t *libdrivers_a = (const uint8_t *)(actual);                                   \
        for (size_t libdrivers_i = 0; libdrivers_i < (size_t)(len); libdrivers_i++) {              \
            if (libdrivers_e[libdrivers_i] != libdrivers_a[libdrivers_i]) {                        \
                LIBDRIVERS_FAIL("%s: byte %zu: expected 0x%02X, got 0x%02X", #actual,              \
                                libdrivers_i, libdrivers_e[libdrivers_i],                          \
                                libdrivers_a[libdrivers_i]);                                       \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/** @brief Human-readable name for a status, for assertion output. */
static const char *libdrivers_test_status_name(Libdrivers_Status_t status) {
    switch (status) {
    case LIBDRIVERS_OK:
        return "LIBDRIVERS_OK";
    case LIBDRIVERS_ERR_BUS:
        return "LIBDRIVERS_ERR_BUS";
    case LIBDRIVERS_ERR_ARG:
        return "LIBDRIVERS_ERR_ARG";
    case LIBDRIVERS_ERR_ID:
        return "LIBDRIVERS_ERR_ID";
    case LIBDRIVERS_ERR_TIMEOUT:
        return "LIBDRIVERS_ERR_TIMEOUT";
    default:
        return "(unknown status)";
    }
}

/** @brief One registered test: its name, for reporting, and its function. */
typedef struct {
    const char *name;
    void (*fn)(void);
} LibdriversTest;

/** @brief Register a test function in a suite's test array. */
#define LIBDRIVERS_TEST(fn)                                                                        \
    { #fn, fn }

/**
 * @brief Run @p tests and report; returns the number of failures.
 *
 * Kept out of the TEST_MAIN macro so the loop is ordinary debuggable code.
 */
static int libdrivers_test_run(const char *suite, const LibdriversTest *tests, size_t count) {
    size_t failures = 0;

    printf("[%s] running %zu tests\n", suite, count);
    for (size_t i = 0; i < count; i++) {
        libdrivers_test_failed = 0;
        tests[i].fn();
        if (libdrivers_test_failed) {
            failures++;
            printf("  x %s\n", tests[i].name);
        } else {
            printf("  . %s\n", tests[i].name);
        }
    }

    if (failures == 0) {
        printf("[%s] all %zu passed\n", suite, count);
    } else {
        printf("[%s] %zu of %zu FAILED\n", suite, failures, count);
    }

    // Clamp: an exit status is 8 bits, and 256 failures must not report success.
    return failures == 0 ? 0 : 1;
}

/** @brief Generate a suite's main(). Place after the test array. */
#define LIBDRIVERS_TEST_MAIN(suite, tests)                                                         \
    int main(void) {                                                                               \
        return libdrivers_test_run(suite, tests, sizeof(tests) / sizeof((tests)[0]));              \
    }

#endif // LIBDRIVERS_TEST_HARNESS_H
