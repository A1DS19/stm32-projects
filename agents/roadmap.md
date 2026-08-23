# Roadmap

## Goal

Work through the "Microcontroller Embedded C Programming" course on real hardware — every
lesson written register-level in `microcontroller-embedded-c-programming/Src/main.c`,
built/flashed/verified from the terminal on the Nucleo-L476RG.

## Milestones — course era

- [x] Workspace reset: old lesson projects deleted, single bare-metal project seeded from
  the empty-project shape, Makefile + VS Code debug wired (2026-08-17)
- [x] Lesson zero verified on hardware: LD2 blink via RCC/GPIOA registers + printf over
  USART2 registers on the VCP; FPU-enable gotcha found and fixed (2026-08-17)
- [x] volatile: optimizer deletes a non-volatile register poll, proven at -O2 (2026-08-22)
- [x] Interrupts: EXTI + SysTick ISRs, shared-data volatile, SWIER1-injected presses (2026-08-22)
- [x] Structs/unions/bit-fields arc: padding, hand-rolled register map vs CMSIS, tagged
  unions, IEEE-754 keyhole, bit-field LED toggle capstone with _Static_assert (2026-08-22)
- [x] UART receive + interactive packet decode (shift+mask vs bit-field, typed over VCP) (2026-08-22)
- [x] First external hardware: 4x4 keypad — matrix scan, real debounce, and the
  keydiag.c MCU-as-continuity-tester that debugged the wiring (2026-08-22)
- [x] Preprocessor finale: macros, # ##, defined(), conditional compilation (2026-08-22)
- [x] **COURSE COMPLETE** — "Microcontroller Embedded C Programming" (Udemy), every
  lesson register-level and verified on the Nucleo-L476RG (2026-08-22)
- [ ] printf decoded in KingstVIS off PA2 (Morpho tap) — carried over from bring-up era

## Done — bring-up era (pre-2026-08-17, projects since deleted; history in git)

- [x] CubeCLT + SWD, headless CubeMX CMake generation, blink/UART/EXTI lessons, CLI and
  VS Code debug, `stm32-new-project` skill, CubeMX2 CLI probe (L4 incompatible by design)

## Later / maybe

- I²C/SPI sensor on the breadboard with protocol decode
- A HAL2-era board lesson via CubeMX2's real CLI
