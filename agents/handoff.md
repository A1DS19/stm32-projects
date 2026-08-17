# Where we left off
_2026-08-17 (workspace reset session)_

## This session
- **Workspace reset**: all old projects deleted (`cli-test`, `hello-world`, `button-exti`,
  `testing-target`, `testing-host`, root Eclipse files, `gen.txt`) in favor of a single
  bare-metal course project `microcontroller-embedded-c-programming/` for the
  register-level "Microcontroller Embedded C Programming" course. Branch
  `feat/course-reset`, three commits (scaffold / deletions / docs), not merged or pushed.
- **New project verified on hardware**: seeded from `test-ext`'s empty-project shape,
  renamed; CMSIS headers from the L4 pack via include paths; `make build flash` +
  `blink N` lines observed on the VCP; LD2 blinking via RCC/GPIOA registers.
- **FPU gotcha found the hard way**: ST's empty-project cmake compiles hard-float but the
  startup's `SystemInit` is a weak empty stub — first `printf("%lu")` hard-faulted in
  newlib `vfprintf` (banner survived: GCC folds no-arg printf to puts). Fix:
  `Src/system.c` `SystemInit` enables CP10/CP11. Recorded in agents/decisions.md.
- Makefile targets now work from repo root (forwarding Makefile) and project dir;
  root `.vscode/` retargeted to the new project.

## State
- Branch `feat/course-reset` (3 commits ahead of local main), unpushed. Merge/PR is the
  user's call.
- Untracked leftovers the user chose to delete manually: `test-ext/` (its skeleton lives
  on inside the new project) — root `.settings/` may also linger (git-ignored).
- `.vscode/launch.json` carries an auto-added "C/C++ Runner: Debug Session" config
  pointing at deleted `test-ext` paths — stale, remove when convenient.
- Board: healthy, running lesson zero (banner + blink counter on VCP). Serial = the
  ttyACM with `ID_MODEL=STM32_STLink` (ttyACM0 is the Turing USB monitor).

## Next session
1. Merge `feat/course-reset` (user decision), delete `test-ext/` manually.
2. First real course lessons in `Src/main.c` (data types / bitwise ops on live registers).
3. Still open from bring-up era: printf decoded in KingstVIS off PA2.
