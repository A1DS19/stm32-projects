# Roadmap

## Goal

A fully IDE-free STM32 workflow on this machine — generate, build, flash, debug, and verify on
real hardware from the terminal — validated on the Nucleo-L476RG.

## Milestones

- [x] CubeCLT installed and talking to the board over SWD (2026-08-15)
- [x] Headless CubeMX generating CMake projects (`cli-test`) (2026-08-15)
- [x] CLI build → flash → verified LD2 blink on hardware (2026-08-15)
- [x] LA2016 logic analyzer set up; 2.5 Hz blink captured and confirmed (2026-08-15)
- [ ] UART demo: `printf` over USART2/ST-LINK VCP, decoded in KingstVIS
- [ ] CLI debug session exercised end to end (ST-LINK_gdbserver + arm-none-eabi-gdb)
- [ ] Commit `cli-test` + docs on a feature branch

## Later / maybe

- Migrate `testing-target` to a standalone CMake build
- I²C/SPI sensor on the breadboard with protocol decode
