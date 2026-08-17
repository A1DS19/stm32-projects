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
- [ ] Course section: data types, variables, bitwise ops on live registers
- [ ] Course section: pointers, casting, memory-mapped I/O
- [ ] Course section: structs/unions for register modeling
- [ ] printf decoded in KingstVIS off PA2 (Morpho tap) — carried over from bring-up era

## Done — bring-up era (pre-2026-08-17, projects since deleted; history in git)

- [x] CubeCLT + SWD, headless CubeMX CMake generation, blink/UART/EXTI lessons, CLI and
  VS Code debug, `stm32-new-project` skill, CubeMX2 CLI probe (L4 incompatible by design)

## Later / maybe

- I²C/SPI sensor on the breadboard with protocol decode
- A HAL2-era board lesson via CubeMX2's real CLI
