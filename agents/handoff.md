# Where we left off
_2026-08-17 (evening — post-reset session wrap-up)_

## This session
- **Workspace reset landed on main**: `feat/course-reset` merged (`da9ad23`), pushed, and
  both stale branches (`feat/course-reset`, `feat/button-exti`) deleted. Workspace is now
  one-directory-per-course; `microcontroller-embedded-c-programming/` is the only project.
  Per-project Makefiles (user decision — no root build plumbing; recorded in decisions.md).
- **Reference docs stocked in `docs/`** (all committed + pushed):
  RM0351 Rev 9 + UM1724 Rev 14 (st.com blocks CLI downloads — TLS fingerprint; fetched
  via UMaine/Purdue university mirrors), six per-peripheral Nucleo-L476RG pinout diagrams
  from stm32python.gitlab.io (visually verified against DS10198 AF tables + UM1724 before
  committing), `course-progression.md` (FastBit path tracker), and a repo-root README.
- **Course context established**: user is 34% through FastBit's Embedded C course; path is
  C → Cortex-M3/M4 → MCU1 → MCU2 → DMA → FreeRTOS → Bootloader, with W5500/BLE as
  post-MCU1 applied projects and MCU3 parked (L476 has no LTDC). Lesson topics this
  session: bit set/clear/toggle idioms (`reg &= ~(7U << 4)`), the `U` suffix, and the
  GPIO pin-mux (MODER + AFR, AF tables) — the user now connects CubeMX's pin dropdown to
  the registers underneath.

## State
- `main` at `a9f5c9d`, clean, even with origin. Only branch.
- Board: healthy, running lesson zero (banner + blink counter on VCP at 115200;
  serial = the ttyACM with `ID_MODEL=STM32_STLink`, usually ttyACM1).
- `tio` exit reminder the user asked about: `Ctrl-t q`.
- Direct-to-main commits became this session's accepted pattern for small docs changes
  (user declined the branch-restore fix and kept pushing main).

## Next session
1. Continue the Embedded C course in `Src/main.c` — next lessons are bitwise ops on live
   registers, which dovetails with the pin-mux discussion; use the pinout diagrams +
   RM0351 now in `docs/`.
2. Update `docs/course-progression.md` statuses as lessons/courses advance.
3. Nothing pending or broken; root `.settings/` still lingers untracked (harmless), and
   `.vscode/launch.json` still carries the stale C/C++ Runner entry pointing at deleted
   `test-ext` paths — remove when convenient.
