# stm32-projects — Context

Embedded firmware workspace for the STM32 Nucleo-L476RG, driven entirely from the CLI.
Reset on 2026-08-17: one directory per online-course project, each self-contained with
its own Makefile (no workspace-level build plumbing).

**embedded-system-programming-on-arm-cortex-m3m4**:
The current course project (started 2026-08-23) — FastBit's Cortex-M3/M4 *processor*
course: operation modes, stack (MSP/PSP), NVIC/faults, SVC/PendSV, a from-scratch task
scheduler, then startup files and linker scripts. Same bare-metal shape as the sibling;
course map + F407→L476RG adaptation notes in its `SYLLABUS.md`.
_Avoid_: "the Cortex course" in docs without naming the directory once first

**microcontroller-embedded-c-programming**:
The first course project — bare-metal (no HAL) register-level, for the "Microcontroller
Embedded C Programming" curriculum. Single `Src/main.c` overwritten per lesson; old
lessons live in git history.
_Avoid_: "the course project" in docs without naming it once first

**Empty-project shape**:
The ST VS Code extension's no-HAL project layout this project was seeded from: flat
`Src/`+`Inc/`, `startup_stm32l476xx.S`, `syscall.c`, `sysmem.c`, own linker script,
modular `cmake/` includes, user-editable sections in `CMakeLists.txt`.
_Avoid_: "CubeMX project" — no `.ioc`, CubeMX is not in the loop

**CubeCLT**:
ST's official command-line toolset (compiler, programmer CLI, GDB server, CMake/Ninja) — the build/flash/debug backbone of this workspace.
_Avoid_: "the toolchain", "CubeIDE tools"

**Headless CubeMX**:
Standalone STM32CubeMX driven by a `-q <script>` command file. Not used by the current
project; kept documented (docs/cli-workflow.md) for future HAL projects.
_Avoid_: "CubeMX CLI mode", "quiet mode"

**Firmware pack**:
An STM32Cube HAL/CMSIS bundle in `~/STM32Cube/Repository/` (here `STM32Cube_FW_L4_V1.18.2`).
The current project uses only its CMSIS headers (`stm32l476xx.h`, `core_cm4.h`) via include
paths in `CMakeLists.txt` — no HAL sources compiled.
_Avoid_: "SDK", "HAL download"

**LA2016**:
The Kingst 16-channel logic analyzer on this bench (KingstVIS software); validates firmware behavior independently of the code.

**B1**:
The Nucleo's blue user push-button on PC13. Pressing pulls the line **low** (active-low).
Note it has a 100nF cap — visible bounce is largely filtered.
_Avoid_: "the button" without naming which, "PC13 switch"; confusing it with the black RESET button
