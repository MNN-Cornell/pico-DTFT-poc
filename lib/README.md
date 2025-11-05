# DTFT Library Modules

This directory contains modularized components of the pico-DTFT-poc project.

## Module Overview

### `lut.h` / `lut.c` - Lookup Tables
- Pre-computed sine/cosine lookup tables (256 entries, 8-byte aligned)
- Fast inline trigonometric functions: `fast_sin()`, `fast_cos()`
- Initialization: `init_trig_lut()`

### `dtft.h` / `dtft.c` - DTFT Computation
- Single-core DTFT: `calculate_dtft()`, `compute_dtft_magnitude()`
- Dual-core DTFT: `calculate_dtft_complex()` (Core0 + Core1 parallel)
- Core1 initialization: `init_core1_dtft()`
- Loop unrolling (4x) and memory barriers for synchronization

### `gpio_control.h` / `gpio_control.c` - GPIO Operations
- LED control: `pico_led_init()`, `pico_set_led()`
- Signal transmission: `init_signal_gpio()`, `send_bit()`, `send_data()`
- GPIO pins: GPIO2 (signal), GPIO3 (clock), GPIO4 (TX_ACTIVE)

### `signal.h` / `signal.c` - Signal Processing
- Pattern repetition: `repeat_pattern()`
- Pattern processing: `process_pattern()` (DTFT + visualization)

### `output.h` / `output.c` - Output & Visualization
- Terminal spectrum plot: `plot_dtft_spectrum()`
- MATLAB export: `print_dtft_complex_for_matlab()`

## Usage

Include the headers in your code:
```c
#include "lib/lut.h"
#include "lib/dtft.h"
#include "lib/gpio_control.h"
#include "lib/signal.h"
#include "lib/output.h"
```

## Build

The CMakeLists.txt automatically includes all library source files:
```cmake
add_executable(poc
    main.c
    lib/lut.c
    lib/dtft.c
    lib/gpio_control.c
    lib/signal.c
    lib/output.c
)
```

## Performance Optimizations

- **Lookup Tables**: 8-byte aligned for ARM Cortex-M33 cache efficiency
- **Inline Functions**: `fast_sin()` and `fast_cos()` use `__attribute__((always_inline))`
- **Loop Unrolling**: 4x unrolling in DTFT computation for instruction-level parallelism
- **Dual-Core**: Core1 handles second half of frequency range while Core0 handles first half
- **Memory Barriers**: `__dmb()` ensures synchronization between cores
