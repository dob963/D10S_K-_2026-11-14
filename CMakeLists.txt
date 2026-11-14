cmake_minimum_required(VERSION 3.16)
project(D10S_Spectrum_Core C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(d10s_spectrum_core firmware/spectrum_core.c)
target_include_directories(d10s_spectrum_core PUBLIC firmware)
target_compile_options(d10s_spectrum_core PRIVATE
    -Wall -Wextra -Werror -pedantic
)

add_executable(test_spectrum_core tests/test_spectrum_core.c)
target_link_libraries(test_spectrum_core PRIVATE d10s_spectrum_core)
target_compile_options(test_spectrum_core PRIVATE
    -Wall -Wextra -Werror -pedantic
)

enable_testing()
add_test(NAME spectrum_core COMMAND test_spectrum_core)
