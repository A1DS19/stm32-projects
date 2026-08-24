# Roadmap

## Goal

Work through the FastBit course path on real hardware — every lesson register-level,
built/flashed/verified from the terminal on the Nucleo-L476RG. Current course:
"Embedded System Programming on ARM Cortex-M3/M4" in
`embedded-system-programming-on-arm-cortex-m3m4/` (course map: its `SYLLABUS.md`).

## Milestones — Cortex-M processor course era

- [x] Course project scaffolded (cloned from the Embedded C project shape) + 375-slide
  deck distilled into `SYLLABUS.md`; banner/LED scaffold verified on hardware (2026-08-23)
- [ ] Sections 1–6: modes, inline assembly, reset sequence, memory map, bit-banding, stack
- [ ] Sections 7–9: NVIC, faults, SVC/PendSV
- [ ] Capstone: round-robin task scheduler (needs the 4-LED substitute decision)
- [ ] Sections 11–12: own startup file + linker script, OpenOCD, newlib
- [ ] **COURSE COMPLETE** → run the course-finish ritual (docs/course-progression.md)

## Milestones — Embedded C course era (complete)

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
