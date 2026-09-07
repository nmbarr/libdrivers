# ---------------------------------------------------------------------------
# Host-side unit tests for the HAL-free core.
#
# The drivers reach hardware only through function-pointer transports, so a
# test can supply its own implementation of one and run the real driver code
# on the host -- no target, no HAL, no hardware. That is what fake_bus and
# fake_onewire are.
#
# One executable per driver, each registered with CTest separately, so a
# failure names the driver it came from and suites run in parallel.
# ---------------------------------------------------------------------------

# The fakes and their headers, shared by every suite.
add_library(test_support STATIC
    fake_bus.c
    fake_onewire.c
)
target_link_libraries(test_support PUBLIC drivers)

# PUBLIC so each suite inherits the path to fake_bus.h / test_harness.h.
target_include_directories(test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The tests exercise the drivers' edge cases deliberately, so they must build
# as cleanly as the drivers do -- a warning here is as much a defect as one in
# src/. Applied to the test targets only; the library sets its own standard.
set(LIBDRIVERS_TEST_WARNINGS -Wall -Wextra -Wpedantic)
if(LIBDRIVERS_WARNINGS_AS_ERRORS)
    list(APPEND LIBDRIVERS_TEST_WARNINGS -Werror)
endif()

foreach(suite bus ds18b20 hts221 icm42688 lis3mdl lps22hb lsm6dsl)
    add_executable(test_${suite} test_${suite}.c)
    target_link_libraries(test_${suite} PRIVATE test_support)
    target_compile_options(test_${suite} PRIVATE ${LIBDRIVERS_TEST_WARNINGS})
    add_test(NAME ${suite} COMMAND test_${suite})
endforeach()
