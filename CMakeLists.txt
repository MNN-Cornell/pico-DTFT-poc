# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Force ARM Cortex-M33 architecture (not RISC-V)
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "Pico platform")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(poc C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(poc
    main.c
    lib/lut.c
    lib/dtft.c
    lib/gpio_control.c
    lib/signal.c
    lib/output.c
    )

# Add include directories for lib modules
target_include_directories(poc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# pull in common dependencies
target_link_libraries(poc pico_stdlib pico_multicore)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(poc pico_cyw43_arch_none)
endif()

# Enable aggressive optimizations for ARM Cortex-M33 (RP2350)
# Note: Pico SDK handles FPU configuration automatically for ARM
target_compile_options(poc PRIVATE
    -O3                          # Maximum optimization
    -ffast-math                  # Aggressive math optimizations (enables FPU optimizations)
    -funroll-loops               # Unroll loops for speed
    -finline-functions           # Aggressive inlining
    -fno-math-errno              # Don't set errno for math functions
    -fno-trapping-math           # No FP exceptions
    -ftree-vectorize             # Enable auto-vectorization
)

# create map/bin/hex file etc.
pico_add_extra_outputs(poc)
pico_enable_stdio_usb(poc 1)
# add url via pico_set_program_url
