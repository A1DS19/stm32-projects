# Roadmap

## Goal

A fully IDE-free STM32 workflow on this machine — generate, build, flash, debug, and verify on
real hardware from the terminal — validated on the Nucleo-L476RG.

## Milestones

- [x] CubeCLT installed and talking to the board over SWD (2026-08-15)
- [x] Headless CubeMX generating CMake projects (`cli-test`) (2026-08-15)
- [x] CLI build → flash → verified LD2 blink on hardware (2026-08-15)
- [x] LA2016 logic analyzer set up; 2.5 Hz blink captured and confirmed (2026-08-15)
- [x] UART demo: `printf` over USART2/ST-LINK VCP (`hello-world`, 2026-08-17 — KingstVIS decode still pending)
- [x] CLI debug session exercised end to end (ST-LINK_gdbserver `-d` + arm-none-eabi-gdb; also VS Code Cortex-Debug with F5 + SVD registers) (2026-08-17)
- [x] Commit `cli-test` + docs on a feature branch (docs + `hello-world` committed and pushed, 2026-08-17)
- [x] `stm32-new-project` skill in claude-configs: scaffolds grill→generate→flash-verified projects (2026-08-17)
- [x] Install CubeMX2 and probe its CLI (2026-08-17: installed 1.1.1 — installer needs
  `WEBKIT_DISABLE_DMABUF_RENDERER=1`; `cube mx` is a real headless CLI, create→generate
  verified on a C5 board, but **L4 packs are `compatibility:incompatible` — can't drive the
  Nucleo-L476RG yet**; details in the skill's `references/generation.md`)
  (Follow-up: L4 exclusion is by design — CubeMX2 targets the new HAL2 architecture;
  legacy lines stay on classic CubeMX. Only relevant if a lesson moves to a HAL2-era part.)
- [ ] printf decoded in KingstVIS off PA2 (Morpho tap)
- [x] button-exti lesson: B1 → EXTI15_10 → LD2 toggle + press counter on VCP, HAL_GetTick
  debounce, 10-for-10 presses verified over serial (2026-08-17; B1's 100nF cap meant raw
  bounce never showed — the BSP flavor of generation, via Board Project Options dialog)

## Later / maybe

- Migrate `testing-target` to a standalone CMake build
- I²C/SPI sensor on the breadboard with protocol decode
- Empty-CMSIS (no-HAL) lesson using the ST extension style (`test-ext` is the reference)
