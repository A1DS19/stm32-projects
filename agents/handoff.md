# Where we left off
_2026-08-30 (Cortex-M course — COMPLETE; sections 10, 11 and 12 all done today)_

## This session (2026-08-30)
- Section 10 (scheduler.c), 11 (`gnu-build/`: own startup + linker script + Makefile,
  OpenOCD/GDB load) and 12 (`gnu-build/` variants: newlib vs nano vs gc vs semihosting,
  `make inspect` / `make run-semi` scripted OpenOCD) — all hardware-verified, see the
  SYLLABUS progress rows for the numbers and findings.
- Bench: the ST-LINK firmware (V2J28M18) had killed SWD; `STLinkUpgrade.sh` → V2J48M35
  fixed it (SYLLABUS bench row + memory `stlink-ap-status-0x5`).
- Course-finish ritual run: docs/course-progression.md (row removed, Completed log, MCU1
  "next up"), roadmap milestones ticked, README + CLAUDE.md now list both courses complete.

## State
- `main` at the section-12 commit, not pushed (`git push` when ready).
- Board: `gnu-build/build/final.elf` (UART build), parked in Default_Handler by design.
  The CMake project's main.c still points at the blocking scheduler.
- `gnu-build/` targets: all/stages/analyze/flash/openocd/load/debug/serial/clean +
  nano/gc/semi/compare/analyze-gc/inspect/run-semi. OpenOCD = CubeIDE 2.0's bundled build.

## Next session
1. **Decide what's next** — per memory `capstone-after-roadmap`: the course roadmap is done,
   so either the culminating project (front-runner: point-to-point LoRa messenger) or the
   next course, MCU1 (driver development — needs a new project dir via /stm32-new-project).
   The user decides; nothing is started.
2. Optional loose ends: LA2016 capture of the scheduler's four pins (PA5/PA6/PA7/PA9); the
   GDB stack-activity log for the scheduler (slide exercise); `git push`.
