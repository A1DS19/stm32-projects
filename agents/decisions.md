# Decisions

## 2026-08-15 — CLI-first toolchain over CubeIDE

All new work uses CubeCLT + headless CubeMX from the terminal; CubeIDE 2.0 stays installed but
only as a fallback GUI. Chosen for scriptability and because CubeIDE 2.0 dropped its embedded
scriptable CubeMX engine, making standalone CubeMX necessary anyway.

## 2026-08-15 — CMake as the generated toolchain

New projects generate with `project toolchain CMake` (presets Debug/Release, Ninja), not
Eclipse-managed builds or bare Makefiles. This is what CubeCLT and ST's VS Code flow target,
and it keeps projects buildable with nothing but the CLI. Legacy Eclipse projects are left
untouched rather than migrated wholesale.

## 2026-08-17 — Workspace reset to a single bare-metal course project

All lesson projects (`cli-test`, `hello-world`, `button-exti`, legacy Eclipse
`testing-target`/`testing-host`) deleted in favor of one project,
`microcontroller-embedded-c-programming`, for the register-level embedded C course.
Rationale: the CubeMX/HAL bring-up era served its purpose (toolchain proven, skill
extracted); the course teaches direct register access, which HAL actively obscures.
History stays in git.

Sub-decisions:
- **Bare-metal, no HAL** — seeded from the ST-extension empty-project shape (`test-ext`)
  rather than regenerated; CMSIS headers come from the installed L4 pack via include paths.
- **Single `main.c`, overwritten per lesson** — chosen over one-file-per-lesson with a
  build selector; git history is the lesson archive.
- **printf over USART2 registers, not ITM/SWO** — the course uses ITM, but UART keeps
  `make serial` and the existing VCP flow working with zero extra tooling. `uart2.c`
  provides `__io_putchar`; `syscall.c` stays stock.
- **`SystemInit` must enable the FPU** — the empty-project startup calls a weak empty
  `SystemInit`, while the ST cmake flags compile hard-float; newlib's `vfprintf` pushes
  FPU registers and hard-faults without CP10/CP11 access. `Src/system.c` owns this.
  (Found on hardware: banner printed — GCC folds no-arg printf to puts — but the first
  `%lu` printf died in the default handler.)
