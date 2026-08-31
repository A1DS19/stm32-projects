# Course progression

Learning path for embedded firmware, anchored on the FastBit STM32 track. One workspace
project directory per course (per-project Makefile). Update the status column as courses
advance; move finished ones to the log at the bottom.

## Main path (in order)

| # | Course | Status | Project dir |
|---|--------|--------|-------------|
| 3 | Mastering Microcontroller and Embedded Driver Development — MCU1 (FastBit) | **next up** (1%) | — |
| 4 | Mastering Microcontroller: Timers, PWM, CAN, Low Power — MCU2 (FastBit) | 1% | — |
| 5 | ARM Cortex M Microcontroller DMA Programming Demystified (FastBit) | 1% | — |
| 6 | Mastering RTOS: Hands on FreeRTOS and STM32Fx with Debugging (FastBit) | 2% | — |
| 7 | STM32Fx Microcontroller Custom Bootloader Development (FastBit) | 1% | — |

Why this order: C fundamentals → the Cortex-M4 core itself (exceptions, NVIC, stack,
faults — the machinery behind the hard-fault/FPU issues already met in lesson zero) →
register-level peripheral drivers (the MODER/AFR pin-mux work, made systematic) → deeper
peripherals → DMA → scheduler → bootloader. Steps 6 and 7 can swap by interest.

Reference alongside step 3 (not a separate step): Communication Protocols: UART, USART,
I2C, I3C, SPI, GPIO (Protocol Pro).

## Hardware notes

- Bench board: Nucleo-L476RG. FastBit videos use STM32F407 Discovery / Nucleo-F446 —
  same Cortex-M4 core, slightly different peripheral versions; adapt via RM0351 instead
  of copy-typing. That adaptation is a feature, not a bug.
- MCU3 (LTDC/LCD-TFT/LVGL, owned, 3%) is the exception: the L476 has no LTDC. Needs an
  F429/F7 Discovery board — park it until then.

## Applied projects (unlocked after MCU1)

Not foundation steps — project courses that apply the driver skills from step 3:

- **Embedded Ethernet on STM32 Using W5500** — W5500 is an external SPI TCP/IP chip;
  ideal first post-MCU1 project. Runs fully on the L476RG + a cheap W5500 module.
- **Bluetooth Low Energy (BLE) From Ground Up** — protocol-focused, self-contained
  (GAP/GATT/advertising). L476 has no radio: needs the course's BLE board/module,
  which is driven over UART/SPI. Independent of MCU2/DMA/RTOS.

## Later / parallel tracks (owned)

- **Zephyr RTOS w/ DeviceTree and Board Bring Up** (in cart) — buy and start only after
  step 6 (FreeRTOS first makes DeviceTree/bring-up land better).
- **Embedded Linux**: Embedded Linux Step by Step (BeagleBone Black, 1%) → Linux Device
  Driver Programming LDD1 (1%). Own track, after the MCU path.
- **Mastering Embedded Rust** (FastBit) — after MCU1/MCU2; same concepts, new language.
- **Advanced Embedded Software with STM32, FreeRTOS & Modbus** — after step 6.
- **PCB/hardware**: KiCad Advanced smart USB thumb drive (4%), electronics design
  courses — parallel to the drone-flight-controller PCB work, independent of this path.
- **FPGA/Verilog series** (cart) — separate discipline; decide later, not part of this path.

## Completed

| Course | Finished | Notes |
|--------|----------|-------|
| Embedded Systems Programming on ARM Cortex-M3/M4 Processor (FastBit) | 2026-08-30 | Step 2 of the main path, one week on the bench. 16 lessons in `embedded-system-programming-on-arm-cortex-m3m4/` (LESSONS.md is the map): modes, core registers, inline asm, reset, memory map, bit-banding, MSP/PSP, NVIC (+CMSIS), faults, SVC, a from-scratch round-robin scheduler, then `gnu-build/` — own startup file, linker script and Makefile, OpenOCD/GDB, newlib vs nano, semihosting. Every lesson hardware-verified; slide errors caught and documented (VECTPENDING, SVCALLPENDED, EXC_RETURN in the frame's lr slot, tick before PSP). Bonus: 7-segment + SSD1306 side demos, a GDB fault autopsy, and the ST-LINK firmware outage diagnosed with OpenOCD |
| Microcontroller Embedded C Programming: Absolute Beginners (FastBit) | 2026-08-22 | Step 1 of the main path. Register-level on the Nucleo-L476RG, no HAL; one lesson per file in `microcontroller-embedded-c-programming/` (main.c points at the current one, git is the archive). Bonus beyond the course: 4x4 keypad integration + MCU-as-continuity-tester (keydiag.c) |
| Curso diseño PCB: Controlador de vuelo para drones Arduino | 2026 (before 2026-08-17) | Fed the drone-flight-controller project |
