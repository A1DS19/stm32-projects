# stm32-projects

STM32 workspace for the Nucleo-L476RG, built around a CLI-first workflow: headless CubeMX
generates CMake projects, CubeCLT builds and flashes them — no IDE in the loop. Legacy
Eclipse-managed CubeIDE projects (`testing-target/`, `testing-host/`) predate this and are
kept as-is.

## Stack

- STM32L476RG (Nucleo-64 board, on-board ST-LINK V2)
- STM32CubeCLT 1.22.0 (`/opt/st/stm32cubeclt_1.22.0/`): arm-none-eabi GCC 14.3, CMake + Ninja, `STM32_Programmer_CLI`, `ST-LINK_gdbserver`
- STM32CubeMX 6.18.1 (`~/STM32CubeMX/`), driven headless with `-q <script>`
- STM32Cube_FW_L4 firmware pack in `~/STM32Cube/Repository/`
- Kingst LA2016 logic analyzer, KingstVIS in `~/Apps/KingstVIS/`

## Run & test

```sh
source /etc/profile.d/cubeclt-bin-path_1.22.0.sh   # login shells get this free; scripts must source it
cmake --preset Debug && cmake --build build/Debug   # from a project dir, e.g. cli-test/
STM32_Programmer_CLI -c port=SWD -w build/Debug/<name>.elf -v -rst   # flash, verify, reset
ST-LINK_gdbserver -p 61234 -d -cp /opt/st/stm32cubeclt_1.22.0/STM32CubeProgrammer/bin   # -d = SWD (required); then: arm-none-eabi-gdb <elf> -ex 'target remote :61234'
```

New project: script CubeMX headless (`loadboard NUCLEO-L476RG allmodes`, `project toolchain CMake`,
`project generate`). Install missing firmware packs first: `swmgr install stm32cube_l4_1.18.2 ask` —
`project generate` silently no-ops without the pack. `-q` mode still pops GUI dialogs when a
display is present; expect to click them.

Full lifecycle recipe (create → build → edit → flash → debug → reconfigure → serial): [docs/cli-workflow.md](docs/cli-workflow.md).

## Conventions

- User code goes only between `/* USER CODE BEGIN/END */` markers — everything else is CubeMX-owned and lost on regeneration from the `.ioc`.
- Peripheral/pin changes go through the project's `.ioc` (CubeMX), then regenerate; never hand-edit generated init code.
- Vocabulary in [agents/CONTEXT.md](agents/CONTEXT.md); hard decisions in [agents/decisions.md](agents/decisions.md); milestones in [agents/roadmap.md](agents/roadmap.md).
- Git: never add AI attribution to commits or PRs — no `Co-Authored-By: Claude …` trailers, no
  "Generated with Claude Code" footers. The user is the sole author in the contributor history.

## Session handoff

- At session start, read agents/handoff.md first if it exists.
- When the user ends a session with "let's continue tomorrow" (or similar), overwrite agents/handoff.md per its format: what was done, current state, plan for next session.
