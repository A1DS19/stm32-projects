# stm32-projects

STM32 workspace for the Nucleo-L476RG, CLI-first: CubeCLT builds, flashes, and debugs —
no IDE in the loop. Hosts one directory per online-course project, each self-contained
with its own Makefile. Completed: `microcontroller-embedded-c-programming/` (Embedded C course)
and `embedded-system-programming-on-arm-cortex-m3m4/` (the Cortex-M4 processor course — modes,
stack, NVIC, faults, scheduler capstone, own startup/linker script; map in its `SYLLABUS.md`).
Next course: MCU1 (driver development), not started — see docs/course-progression.md.
Both bare-metal (no HAL) register-level: each lesson in its own `Src/<lesson>.c` (+
`Inc/` header), `main.c` is a thin dispatcher pointing at the current one, git history
is the archive.

## Stack

- STM32L476RG (Nucleo-64 board, on-board ST-LINK V2)
- STM32CubeCLT 1.22.0 (`/opt/st/stm32cubeclt_1.22.0/`): arm-none-eabi GCC 14.3, CMake + Ninja, `STM32_Programmer_CLI`, `ST-LINK_gdbserver`
- CMSIS headers only (no HAL sources) from `~/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/`, wired as include paths in the project's `CMakeLists.txt`
- Kingst LA2016 logic analyzer, KingstVIS in `~/Apps/KingstVIS/`

## Run & test

Each project carries its own Makefile — `cd` into the project dir and:

| Command | Does |
|---|---|
| `make build` | CMake preset Debug + ninja; prints memory usage |
| `make flash` | program + verify + reset over SWD |
| `make serial` | open the VCP terminal (resolves port by USB identity — never assume ttyACM0) |
| `make gdb-server` / `make debug` | two-terminal step debugging (server carries the required `-d`) |
| `make size` / `symbols` / `disasm` | inspect the ELF |
| `make reset` / `clean` | restart board / wipe build |

F5 in VS Code = build, flash, halt at `main` (Cortex-Debug, SVD registers).

`embedded-system-programming-on-arm-cortex-m3m4/gnu-build/` (sections 11–12) has its own hand
Makefile, no CMake: `make`, `make stages`, `make analyze`, `make flash`, the course's OpenOCD
path `make openocd` + `make load`, the library variants `make nano` / `gc` / `semi` / `compare`,
and the scripted OpenOCD runs `make inspect` / `make run-semi` (OpenOCD comes bundled with
CubeIDE 2.0).

## Project layout & conventions

- Empty-project shape (seeded from the ST VS Code extension): `Src/` (`main.c`,
  `startup_stm32l476xx.S`, `syscall.c`, `sysmem.c`, `system.c`, `uart2.c`), `Inc/`,
  `stm32l476xg_flash.ld`, modular `cmake/`.
- `cmake/*.cmake` and the non-user sections of `CMakeLists.txt` are managed-file style —
  add sources/includes/defines only in the marked "User defined" sections of `CMakeLists.txt`.
- `Src/system.c` owns `SystemInit`: it enables the FPU (CP10/CP11). Do not remove — the
  project compiles hard-float and newlib printf hard-faults without it.
- `Src/uart2.c` is printf's backend (USART2 PA2/PA3, 115200 8N1, register-level via
  `__io_putchar`). Lessons call `uart2_init()` first if they print. End printf lines `\r\n`.
- No CubeMX, no `.ioc` — peripheral setup is hand-written register code, that's the point
  of the course. (Headless-CubeMX recipe for future HAL projects: [docs/cli-workflow.md](docs/cli-workflow.md).)
- Vocabulary in [agents/CONTEXT.md](agents/CONTEXT.md); hard decisions in [agents/decisions.md](agents/decisions.md); milestones in [agents/roadmap.md](agents/roadmap.md).
- Git: never add AI attribution to commits or PRs — no `Co-Authored-By: Claude …` trailers, no
  "Generated with Claude Code" footers. The user is the sole author in the contributor history.

## Session handoff

- At session start, read agents/handoff.md first if it exists.
- When the user ends a session with "let's continue tomorrow" (or similar), overwrite agents/handoff.md per its format: what was done, current state, plan for next session.
