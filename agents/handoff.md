# Where we left off
_2026-08-30 (Cortex-M course — sections 10 and 11 written; the bench probe is down)_

## This session (2026-08-30)
- **Section 10 — task scheduler — done and hardware-verified** (`scheduler.c`, merged
  `2945ac7`): PendSV switch with per-task EXC_RETURN + s16-s31, blocking vs spin measured
  live (2013 vs 8612 ticks/period, idle 2000 vs 0). Pins PA5/PA6/PA7/PA9.
- **Section 11 — bare-metal build — written and host-verified** (`gnu-build/`): hand
  Makefile, `startup.c`, `stm32l476rg.ld` from scratch, `openocd.cfg` for CubeIDE 2.0's bundled
  OpenOCD (`…/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.linux64_*/tools/bin/openocd`
  + `…/mcu.debug.openocd_*/resources/openocd/st_scripts`). `make` + `make analyze` prove the
  ELF (entry, vector words, .data LMA≠VMA, weak-alias override). Details in SYLLABUS progress.
- **Bench: SWD down since the section-10 spin-mode flash.** Every ST tool mode fails "Unable
  to get core ID"; OpenOCD gets further — DPIDR 0x2ba01477 reads fine, then every AP access
  returns `unknown/unexpected STLINK status code 0x5` → the probe (firmware **V2J28M18**,
  ancient) or the MCU's debug domain, NOT the SWD wiring (DP answers) and NOT firmware (fails
  under NRST too). USB-level reset of the ST-LINK did not help. Meanwhile NRST, VCP, Vtarget
  3.25 V all fine; the board runs the scheduler's spin build.

## State
- `main` @ section-11 commit (see `git log`), not pushed.
- Board: scheduler spin-mode build running. Pending re-flashes when the probe is back:
  (1) `cd embedded-system-programming-on-arm-cortex-m3m4 && make flash` (main.c = blocking
  scheduler); (2) `cd gnu-build && make flash` + serial → expected: sections map, variable
  table, "42 in two places", vector words, then parks in Default_Handler on IRQ 5 (IPSR=21);
  (3) the course's path: `make openocd` in one terminal, `make load` in another.
- Handler claims unchanged from section 10; `gnu-build/` is a separate link (its startup.c
  aliases everything to Default_Handler; faulthandler.c's HardFault is linked in).

## Next session
1. **Fix the probe first**: unplug/replug the Nucleo USB (full power cycle). If SWD still
   fails, upgrade the ST-LINK firmware (V2J28 → current) with CubeCLT's
   `STM32CubeProgrammer/bin/STLinkUpgrade.jar` (`java -jar …`) — user's call. Then run the
   three pending flashes above and move the section-11 progress row from "host-verified" to
   verified (add the serial findings).
2. **Section 12: OpenOCD, newlib & semihosting** (slides 345–378) — `gnu-build/` already has
   the OpenOCD server/client targets; add newlib vs newlib-nano size comparison
   (`--specs=nano.specs`), `--gc-sections` before/after, semihosting via OpenOCD
   (`arm semihosting enable`) or `ST-LINK_gdbserver --semihosting`, `__libc_init_array` story
   (already in startup.c). That closes the course → run docs/course-progression.md ritual.
3. Ritual per lesson unchanged: "When you'll use this" header + LESSONS.md row + SYLLABUS
   checkbox/progress row + hardware verify + commit (no AI attribution).
