# Where we left off
_2026-08-30 (Cortex-M course, section 10 — the scheduler capstone — done and hardware-verified)_

## This session (2026-08-30)
- **Section 10 — task scheduler — done**, branch `feat/scheduler-lesson` merged into main:
  `scheduler.c` = 4 tasks + idle on 2 KB `.bss` stacks, SysTick 1 ms (RELOAD 3999 from the
  4 MHz MSI), naked PendSV switch saving `{r4-r11, EXC_RETURN}` + `s16-s31` per task (the
  FreeRTOS CM4F shape — required because EXC_RETURN bit 4 is per-task on our hard-float
  build), forged 17-word initial contexts (argument in r0, T bit in xPSR, task's lr → trap),
  TCBs, `task_delay` blocking, WFI idle; two modes in one file (`SCHED_SPIN_DELAYS` vs
  `SCHED_BLOCKING_DELAYS`), main.c runs the blocking one.
- Evidence (serial, task 1 is the only printer): blocking on→on = 2013 ticks, idle = 2000
  per period, T1 FPCA 0→1 after its first printf, T2's saved EXC_RETURN 0xFFFFFFFD→0xFFFFFFED
  after one float multiply, T3/T4 stay basic; spin mode 8612 ticks, idle 0 (×4 stretch +
  ~9 % switch overhead at 4 MHz -O0).
- 4-LED substitute decided: PA5 (LD2/D13) + PA6 (D12) / PA7 (D11) / PA9 (D8), all CN5,
  BSRR writes. User-side LA2016 capture of those pins (2 s / 1 s / 500 ms / 250 ms periods)
  not yet done — optional.
- `nvic.c`'s `PendSV_Handler` renamed `nvic_pendsv_demo` (exported in nvic.h); the
  scheduler's handler forwards to it while `sched_started == 0`, so lesson 10 still runs.

## State
- Board is running the **spin-mode** build (last successful flash); the final
  blocking-mode re-flash failed: every SWD mode (normal/UR/hotplug/100 kHz, USB reset)
  gives "Unable to get core ID" while NRST resets the board, VCP streams and Vtarget reads
  3.25 V — SWDIO/SWCLK path or ST-LINK, not firmware. Bench checklist: board off the
  breadboard (Morpho pins), CN2 jumpers both ON, probes/wires off CN7 13-16, USB
  power-cycle. Then `make flash` once more (main.c is already blocking mode).
  WFI in idle did not disturb the two earlier flashes (-rst).
- Handler/IRQ claims: 3 modes, 4 access, 6 stackmem, 28/31/39 nvic.c, 29 cmsisnvic.c,
  SVC_Handler svc.c, Usage/Bus/MemManage faults.c, HardFault faulthandler.c,
  **PendSV_Handler + SysTick_Handler scheduler.c**.
- Serial-verify recipe: `stty -F /dev/ttyACM1 115200 raw -echo`, background
  `timeout N cat /dev/ttyACM1 > log`, then `make flash` (resets), read the log.
- Not pushed — `git push` when ready.

## Next session
1. **Section 11: bare-metal build with GNU tools** (slides 277–344) — own startup file,
   C startup code, `.o` sections with objdump, linker script from scratch, OpenOCD + GDB
   load (OpenOCD is not installed yet; the workspace flashes via STM32_Programmer_CLI —
   decide whether to install it or map the lesson onto ST-LINK_gdbserver).
2. Optional follow-up for section 10, as a `Log:` commit like the GDB fault autopsy:
   the slide exercise "instruction-level stack debugging" — break in PendSV_Handler, watch
   PSP hop between `task_stacks[i]`, dump a saved 17-word context with its EXC_RETURN.
3. Ritual per lesson unchanged: "When you'll use this" header + LESSONS.md row + SYLLABUS
   checkbox/progress row + hardware verify + commit (no AI attribution).
