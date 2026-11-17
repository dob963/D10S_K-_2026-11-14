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
    src/ui/waterfall.c
    src/ui/spectrum_key.c
    src/app/spectrum_app.c
    src/platform/mono128x64.c
    src/platform/st7565.c
    src/platform/py32f071_uvk1_v3.c
    src/radio/bk4829_bus.c
    src/radio/bk4829.c
    src/radio/bk4829_timing.c
    src/radio/bk4829_scanner_adapter.c
    src/radio/bk4829_uvk1_v3_profile.c
    src/radio/bk4829_uvk1_v3_radio.c
)
target_include_directories(d10s_spectrum_core PUBLIC src/spectrum src/ui src/platform src/radio src/app)
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

add_executable(test_spectrum_view tests/test_spectrum_view.c)
target_link_libraries(test_spectrum_view PRIVATE d10s_spectrum_core)
target_compile_options(test_spectrum_view PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME spectrum_view COMMAND test_spectrum_view)

add_executable(test_spectrum_app tests/test_spectrum_app.c)
target_link_libraries(test_spectrum_app PRIVATE d10s_spectrum_core)
target_compile_options(test_spectrum_app PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME spectrum_application COMMAND test_spectrum_app)

add_executable(test_bk4829_profile tests/test_bk4829_profile.c)
target_link_libraries(test_bk4829_profile PRIVATE d10s_spectrum_core)
target_compile_options(test_bk4829_profile PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME bk4829_uvk1_v3_profile COMMAND test_bk4829_profile)

add_executable(test_py32_clock tests/test_py32_clock.c)
target_link_libraries(test_py32_clock PRIVATE d10s_spectrum_core)
target_compile_options(test_py32_clock PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME py32_clock COMMAND test_py32_clock)

add_executable(test_waterfall tests/test_waterfall.c)
target_link_libraries(test_waterfall PRIVATE d10s_spectrum_core)
target_compile_options(test_waterfall PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME waterfall_dither COMMAND test_waterfall)

add_executable(test_spectrum_key tests/test_spectrum_key.c)
target_link_libraries(test_spectrum_key PRIVATE d10s_spectrum_core)
target_compile_options(test_spectrum_key PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME spectrum_key_debounce COMMAND test_spectrum_key)

add_executable(test_target_defaults tests/test_target_defaults.c src/spectrum/spectrum_plan.c)
target_include_directories(test_target_defaults PRIVATE target src/spectrum)
target_compile_options(test_target_defaults PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
add_test(NAME target_scan_defaults COMMAND test_target_defaults)
