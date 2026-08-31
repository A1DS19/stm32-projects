# Where we left off
_2026-08-30 (Cortex-M course — sections 10 and 11 done and hardware-verified; probe fixed)_

## This session (2026-08-30)
- **Section 10 — task scheduler — done** (`scheduler.c`, `2945ac7`): PendSV switch with
  per-task EXC_RETURN + s16-s31, blocking vs spin measured live. Pins PA5/PA6/PA7/PA9.
- **Section 11 — bare-metal build — done** (`gnu-build/`, `b419bb5` + docs commit): hand
  Makefile, `startup.c`, `stm32l476rg.ld` from scratch, `openocd.cfg`. Board-verified two
  ways: `make flash` (STM32_Programmer_CLI) and the course's `make openocd` + `make load`
  (CubeIDE 2.0's bundled OpenOCD, GDB `load`) — identical serial output, ends parked in
  Default_Handler on IRQ 5 (IPSR=21). Findings in SYLLABUS progress rows.
- **Bench outage solved**: the ST-LINK firmware (V2J28M18) was the culprit — DP answered,
  every AP access returned STLINK status 0x5. `/opt/st/stm32cubeclt_1.22.0/STLinkUpgrade.sh`
  (headless, `-update`) → V2J48M35 and SWD came straight back. Recorded in SYLLABUS + memory.

## State
- `main` up to date with the section-11 docs commit, not pushed.
- Board: running `gnu-build/build/final.elf`, parked in Default_Handler by design. The CMake
  project's main.c points at `playing_with_scheduler(SCHED_BLOCKING_DELAYS)` and was
  re-flashed and verified before that.
- Handler claims unchanged (scheduler.c owns PendSV + SysTick in the CMake project;
  `gnu-build/` is a separate link).
- Serial-verify recipe: resolve the VCP by udev identity (`ID_MODEL=STM32_STLink` — it was
  /dev/ttyACM1 again after the re-enumeration), `stty … 115200 raw -echo`, background
  `timeout N cat`, then flash. Gotcha from tonight: never `pkill -f` a pattern that appears
  in your own shell command line (`pkill -x openocd` instead).

## Next session
1. **Section 12: OpenOCD, newlib & semihosting** (slides 345–378), the last one. Build on
   `gnu-build/`: newlib vs newlib-nano size comparison (`--specs=nano.specs`,
   `-u _printf_float`), `--gc-sections` + `-ffunction-sections` before/after in the map,
   semihosting printf through OpenOCD (`arm semihosting enable`, `--specs=rdimon.specs`
   → `initialise_monitor_handles`) and/or `ST-LINK_gdbserver --semihosting`,
   `__libc_init_array` is already told in startup.c. Then the course-finish ritual
   (docs/course-progression.md: status column + Completed log + roadmap) and commit.
2. After the course: the capstone decision (memory: capstone-after-roadmap — front-runner
   point-to-point LoRa messenger).
3. Ritual per lesson unchanged: "When you'll use this" header + LESSONS.md row + SYLLABUS
   checkbox/progress row + hardware verify + commit (no AI attribution).
