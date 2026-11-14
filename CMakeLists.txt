cmake_minimum_required(VERSION 3.16)
project(D10S_Spectrum_Core C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(d10s_spectrum_core
    src/spectrum/spectrum_detect.c
    src/spectrum/spectrum_plan.c
    src/spectrum/spectrum_scanner.c
    src/ui/spectrum_view.c
    src/platform/mono128x64.c
    src/platform/st7565.c
    src/radio/bk4829_bus.c
    src/radio/bk4829.c
    src/radio/bk4829_timing.c
)
target_include_directories(d10s_spectrum_core PUBLIC src/spectrum src/ui src/platform src/radio)
target_compile_options(d10s_spectrum_core PRIVATE -Wall -Wextra -Werror -pedantic)

add_executable(test_spectrum_detect tests/test_spectrum_detect.c)
target_link_libraries(test_spectrum_detect PRIVATE d10s_spectrum_core)
target_compile_options(test_spectrum_detect PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)

enable_testing()
add_test(NAME spectrum_detector COMMAND test_spectrum_detect)

add_executable(test_bk4829_bus tests/test_bk4829_bus.c)
target_link_libraries(test_bk4829_bus PRIVATE d10s_spectrum_core)
target_compile_options(test_bk4829_bus PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME bk4829_bus COMMAND test_bk4829_bus)

add_executable(test_st7565 tests/test_st7565.c)
target_link_libraries(test_st7565 PRIVATE d10s_spectrum_core)
target_compile_options(test_st7565 PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME st7565_transport COMMAND test_st7565)

add_executable(test_bk4829_scanner_adapter tests/test_bk4829_scanner_adapter.c)
target_link_libraries(test_bk4829_scanner_adapter PRIVATE d10s_spectrum_core)
target_compile_options(test_bk4829_scanner_adapter PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME bk4829_scanner_adapter COMMAND test_bk4829_scanner_adapter)
