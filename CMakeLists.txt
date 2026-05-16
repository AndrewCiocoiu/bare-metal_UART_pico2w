cmake_minimum_required(VERSION 3.13)

# Initialize the SDK
include(pico_sdk_import.cmake)

project(bare_metal_blink C CXX ASM)

pico_sdk_init()

# Tell CMake which C file to compile
add_executable(bare_metal_blink main.c)

# We link pico_stdlib ONLY to get the boot header and linker script!
# We are not actually using its functions in our main.c
target_link_libraries(bare_metal_blink pico_stdlib)

# Tell the SDK to generate the magical .uf2 file
pico_add_extra_outputs(bare_metal_blink)