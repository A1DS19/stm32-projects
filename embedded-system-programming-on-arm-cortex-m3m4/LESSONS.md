# Lesson progression

The lessons in the order they were written and should be read. One lesson = one
`Src/<file>.c` (+ `Inc/<file>.h`); `main.c` dispatches to whichever is current, git
history is the archive. Each file's header comment carries the full story, including a
"When you'll use this" section; this table is the map. Course-section detail and
progress log live in [SYLLABUS.md](SYLLABUS.md).

To run an old lesson: point the `#include` and the call in `Src/main.c` at it,
then `make build flash serial`.

| # | File | Teaches | When you'll use it |
|---|---|---|---|
| 1 | `modes.c` | Thread vs handler mode; IPSR; pending an IRQ from software (NVIC ISER/STIR) | Writing/debugging any interrupt code; "which handler am I in?" during crash hunts |
| 2 | `access.c` | Privileged vs unprivileged (CONTROL.nPRIV); the one-way door down; handler mode is always privileged | The user/kernel wall: RTOS task containment; decoding "works in main, faults in task" |
| 3 | `coreregs.c` | R0–R15, xPSR, MSP/PSP, PRIMASK, CONTROL; memory-mapped vs named-only registers; MRS/MOV | Reading disassembly, fault dumps, debugger register panes; context switching IS these registers |
| 4 | `inlineasm.c` | GCC `asm(template : out : in : clobbers)` operand contract; MRS/MSR; LDR/STR exercise (s39) | The five-line assembly islands in every RTOS port: special registers, WFI, barriers, naked functions |
| 5 | `resetseq.c` | vector[0]=initial MSP, vector[1]=Reset_Handler; boot alias at 0; startup's walk to main; VTOR | Bootloaders and firmware update (app jump = hand-made reset); dead-board bring-up triage |
| 6 | `tbit.c` | The Thumb bit: BX/BLX copy address bit 0 into T; INVSTATE fault when 0 | Function pointers, jump tables, bootloader jumps, forged task frames; instant INVSTATE diagnosis |
| 7 | `memmap.c` | ARM's fixed 4 GB map (CODE/SRAM/peripheral/PPB…); AHB vs APB by address; SRAM2's CODE-region twist | Every register address ever typed; linker regions; crash triage by address alone |
| 8 | `bitband.c` | Bit-band alias regions: one word per bit, atomic; the read-modify-write race (exercise s76, safely) | Interrupt-shared flags without disabling interrupts; understanding THE embedded concurrency bug |
| 9 | `stackmem.c` | Full-descending proof; MSP/PSP banking via naked MSR PSP + SPSEL; handler-on-MSP; AAPCS live | The foundation of every RTOS (tasks on PSP, kernel on MSP); stack sizing and overflow forensics |
| 10 | `nvic.c` | The exception model: system exceptions (SCB SHCSR/ICSR/SHPR) vs IRQs (NVIC ISER/ISPR/IPR); software pending; 16 priority levels; preemption, tail-chaining, PRIGROUP, tie-break, PendSV demoted below IRQs — all proven live | Step 2 of every driver ever (enable the line, pick a priority); latency/starvation bug triage; PendSV-at-the-bottom = the scheduler capstone's seed |

## Side demos (register model applied to real hardware)

| File | What | When you'll use it |
|---|---|---|
| `sevenseg.c` | 3461AS 4-digit display multiplexed over raw GPIO — we ARE the driver chip; BSRR atomic writes | Anything hanging straight off pins: LED matrices, relays, keypads |
| `oled.c` + `i2c3.c` | 128x32 SSD1306 over register-level I2C3 — pixels OUTSIDE the map, reached through a bus peripheral | The universal probe/init/transfer pattern for sensors, EEPROMs, displays, radios |

## Shared infrastructure

| File | Role |
|---|---|
| `uart2.c` | printf's backend (USART2 → ST-LINK VCP) + blocking/non-blocking receive |
| `faulthandler.c` | The single strong HardFault_Handler: prints CFSR decoded; lessons crash into it on purpose |
| `system.c` | SystemInit: FPU on (hard-float printf hard-faults without it — do not remove) |
