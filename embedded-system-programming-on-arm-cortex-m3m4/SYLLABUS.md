# Course map — Embedded System Programming on ARM Cortex-M3/M4 (FastBit)

Distilled from the course slide deck (`slides.pdf`, 375 slides, 2024 edition — kept out
of git, it's paid course material). Check sections off as they land; each lesson's code
goes in `Src/` and git history is the archive, same as the last course. The ordered
lesson index (what each file teaches and when it's used) is [LESSONS.md](LESSONS.md).

**The one-line pitch:** the previous course was about the C language on a
microcontroller; this one is about the *processor itself* — the Cortex-M4 core that sits
inside the STM32: its modes, stack, interrupt machinery, faults, and enough of that to
build a working task scheduler from scratch, then the GNU toolchain underneath it all.

## Hardware adaptation (read once)

The course videos use an STM32F407 Discovery board; the bench board is the
Nucleo-L476RG. Both are Cortex-M4 cores, so every *processor* topic (the bulk of this
course) translates 1:1. Chip-side differences to remember:

| Course assumes | On the L476RG |
|---|---|
| 16 MHz HSI clock at boot | 4 MHz MSI at boot — redo every SysTick/delay count |
| 4 user LEDs (PD12–PD15) | 1 user LED (LD2, PA5) — the scheduler drives PA5 (LD2/D13) + PA6/PA7/PA9 (D12/D11/D8), all on header CN5: LEDs on the breadboard through jumper wires (board OFF the breadboard — Morpho pins short), or LA2016 probes; task 1's serial report carries the timing evidence either way |
| ITM/SWO printf in the IDE | `uart2_init()` + printf over the VCP, as always |
| STM32F4 memory/peripheral addresses | Same layout class; check RM0351 for exact addresses |
| Keil / CubeIDE | Our CLI flow (`make build flash serial`, F5 in VS Code) |

Bit-banding, USART3, MSP/PSP, all faults, SVC, PendSV: present on the L476 — no gaps.

## Instructor's code

The course's official repo is cloned at `course-code/` (gitignored, like the slides):
<https://github.com/niekiran/CortexMxProgramming> — 16 numbered STM32CubeIDE projects
for the F407, one per lesson block. Read them as the "answer key", then write our own
register-level L476 version; don't copy-build them (CubeIDE projects, wrong chip).

| Folders | Syllabus section |
|---|---|
| 001–002 HelloWorld (+semihosting) | 1 intro; 12 semihosting |
| 003–006 operation modes, inline, access levels, T-bit | 2 |
| 007 bit_banding | 5 |
| 008 stack | 6 |
| 009 USART3_int_pend, 010 interrupt_priority | 7 |
| 011 exception_entry_exit, 012 fault_gen | 8 |
| 013 svc_number, 014 svc_operation | 9 |
| 015 task_scheduler, 016 cmsis_task_scheduler | 10 (016 = same capstone rewritten with CMSIS helpers) |

Bonus in `course-code/Documents/`: the ARM Cortex-M3 and M4 **Generic User Guides** —
the register-level reference the NVIC/SCB/fault lessons keep citing. Workspace `docs/`
has the M4 *datasheet* and *Technical Reference Manual*; the Generic User Guide is the
third, most lesson-friendly view (programmer's model + register descriptions).

## Sections

- [ ] **1. Intro & motivation** (slides 1–31) — what the course covers; why Cortex-M.
- [x] **2. Operation modes, access levels, inline assembly** (32–48) — thread vs handler
  mode, privileged vs unprivileged; CONTROL and the other non-memory-mapped core
  registers; GCC inline assembly syntax (`asm volatile`, constraints).
  *Exercise (s39): load 2 values from memory, add, store back — pure inline assembly.*
- [x] **3. Reset sequence** (49–53) — what the core does from power-up to `main`:
  MSP fetch from address 0, reset handler, why the vector table starts with an SP value.
- [x] **4. Memory map & buses** (54–69) — the fixed 4 GB map (CODE/SRAM/peripheral/PPB
  regions), AHB vs APB, the chip block diagram.
- [x] **5. Bit-banding** (70–78) — alias addresses that turn one bit into one word;
  atomic bit writes without read-modify-write.
  *Exercise (s76): write 0xFF to 0x2000_0200, clear bit 7 both ways, compare.*
- [x] **6. Stack memory** (79–117) — MSP vs PSP, the four stack models (we use
  full-descending), switching thread mode to PSP, AAPCS (which registers a C function
  may trash, how arguments travel), stacking/unstacking on interrupt.
  *Exercise: instruction-level debugging of stack activity; change SP to PSP.*
- [x] **7. Interrupts & the NVIC** (118–157) — SCB, enabling faults and PendSV, the
  steps to wire any MCU peripheral interrupt, priority values vs urgency, priority
  grouping (pre-empt vs sub-priority), the 16 priority levels ST implements.
  *Exercise (s128): enable and pend the USART3 interrupt from software.*
  *Exercise (s154): interrupt priority configuration.*
- [x] **8. Exception entry/exit & faults** (158–202) — the exception stack frame,
  EXC_RETURN magic values, hard/mem-manage/bus/usage faults and what raises each,
  fault escalation, reading HFSR/CFSR/MMFAR/BFAR, `__attribute__((naked))` handlers
  that pull the faulting PC out of the stacked frame.
  *Exercise (s186): enable all configurable faults, then cause them on purpose —
  undefined instruction, divide by zero, executing from the peripheral region.*
- [x] **9. SVC & PendSV** (203–220) — supervisor calls, extracting the SVC number from
  the stacked PC, PendSV as the context-switch workhorse, interrupt offloading.
  *Exercise (s211): SVC handler prints its number, returns number+4.*
  *Exercise (s212): 4-operation calculator where the SVC number picks the operation.*
- [x] **10. Capstone: task scheduler** (221–276) — the big one. Four user tasks with
  their own stacks, round-robin switching via SysTick + PendSV, dummy initial stack
  frames, task control blocks, a blocking (delay) state, an idle task, a global tick
  count. This is a hand-rolled miniature of what FreeRTOS does — the course's payoff.
  *L476RG: PA5 (LD2) + PA6/PA7/PA9 stand in for the 4 LEDs; SysTick RELOAD 3999 from the 4 MHz MSI; task stacks are .bss arrays, not addresses carved from RAM_END.*
- [x] **11. Bare-metal build with GNU tools** (277–344) — cross-compilation without an
  IDE: compiler options, analyzing `.o` files with objdump, sections (.text/.data/.bss),
  writing a startup file from scratch (vector table via attributes, weak/alias
  handlers, copying .data to RAM, zeroing .bss), writing the linker script (MEMORY,
  SECTIONS, location counter, linker symbols, ALIGN).
  *Done as `gnu-build/`: a self-contained folder with its own hand Makefile, `startup.c`,
  `stm32l476rg.ld` and `openocd.cfg` (CubeIDE's bundled OpenOCD — nothing to install);
  ST's startup/linker script stay in the CMake project untouched.*
- [x] **12. OpenOCD, newlib & semihosting** (345–378) — downloading/debugging with
  OpenOCD, newlib vs newlib-nano, the low-level syscalls printf needs, `--gc-sections`,
  semihosting, `__libc_init_array`.
  *Done inside `gnu-build/`: `make nano` / `gc` / `semi` / `compare`, `make inspect` and
  `make run-semi` (scripted OpenOCD — CubeIDE 2.0's bundled build, nothing installed).
  `ST-LINK_gdbserver --semihosting` exists as the ST alternative, not used.*

## Progress

| Date | Section | Notes |
|---|---|---|
| 2026-08-23 | Scaffold | Project cloned from the Embedded C course shape; banner verified on hardware |
| 2026-08-23 | 2 (started): operation modes | Thread→handler→thread proven on hardware via NVIC STIR-pended IRQ 3, IPSR printed 0→19→0 |
| 2026-08-23 | 2: access levels (PAL vs nPAL) | CONTROL.nPRIV drop → NVIC touch hard-faults; fault handler reads ISER0 fine (handler mode always privileged). Bonus: CONTROL=5 — FPCA set by hard-float printf |
| 2026-08-23 | 2: core registers + mapped vs not | MOV/MRS snapshot printed (PC/LR in flash, SP in SRAM, PSP=0) vs pointer reads (CPUID/ISER0/AHB2ENR). Hardware lessons: MRS reads EPSR slice (T-bit) as 0; MSP snapshot sits one call-frame below SP |
| 2026-08-23 | 2 done: inline assembly | asm(template:out:in:clobbers) contract; s39 exercise via LDR/LDR/ADD/STR on &var addresses (not raw 0x2000_xxxx — our data lives there); MRS/MSR PRIMASK toggle 0→1→0. **Section 2 complete** |
| 2026-08-24 | 3 done: reset sequence | vector[0]==&_estack, vector[1]==&Reset_Handler (Thumb bit), boot alias @0 mirrors both; our startup's walk to main explained. Finding: VTOR still 0 — ISRs fetched via the alias; a bootloader must move it. **Section 3 complete** |
| 2026-08-24 | 2 addendum: T bit (006) | bit0 of &innocent_function cleared → BLX copies 0 into T → INVSTATE, CFSR=0x00020000 on serial. HardFault reporter extracted to shared faulthandler.c (single strong symbol; prints CFSR + the handler-mode-privilege proof) |
| 2026-08-24 | 4 done: memory map & buses | memmap.c probes this program against the map: function/alias-word → CODE, .data/.bss/stack → SRAM, USART2/RCC/GPIOA classified APB1/AHB1/AHB2 by address alone, CPUID → PPB. Punchline: ST's SRAM2 at 0x1000_0000 — writable RAM inside ARM's CODE region (no clock enable needed); regions are zoning, vendors pick tenants. **Section 4 complete** |
| 2026-08-24 | side demo: 3461AS 7-segment | sevenseg.c — the register model made visible: 12 GPIO pins multiplex 4 digits (segments=GPIOB single BSRR write, commons=GPIOA), typed serial digits go RDR(APB1)→BSRR(AHB2)→photons. All wiring on silkscreened Arduino pins; SEVENSEG_COMMON_ANODE flag for a 3461BS |
| 2026-08-24 | side demo: 128x32 SSD1306 OLED | oled.c + i2c3.c — the opposite of sevenseg: pixels belong to a controller chip OUTSIDE the map, reached through I2C3 (0x4000_5C00, PC0/PC1 = A5/A4, AF4, 100 kHz from 4 MHz). Probe-until-ACK loop = hot-wirable; NACK path verified electrically (address frames go out, STOPF lands). Framebuffer+5x7 font in our SRAM, shipped in 4 bursts |
| 2026-08-24 | 5 done: bit-banding | s76 on &bucket, not raw 0x2000_0200 (demo reads it: holds our own .data — 0x200001f8 lives there). RMW clear vs one aliased store, both → 0x7F; alias reads return single bits; peripheral alias proven on USART2 CR1.UE (0x42088000 reads 1). GPIO out of reach (0x4800_0000 > bandable MB) → that's BSRR's job. Gotcha: TXE reads 0 right after printf (TDR still full) — pick deterministic bits for demos. **Section 5 complete** |
| 2026-08-24 | 6 done: stack memory | stackmem.c — FD proven live (single asm PUSH probe: SP −4, [SP]=sentinel; replaces the IDE stepping exercise); naked MSR PSP + CONTROL.SPSEL switch (CONTROL 4→6, FPCA preserved); a local's address moves inside our 2K process_stack array; software-pended EXTI0 handler's local back in MSP territory + CONTROL reads SPSEL=0 in handler; AAPCS callee-saved proven with r4-pinned sentinel surviving printf. PSP=0-since-reset breadcrumb cashed. **Section 6 complete** |
| 2026-08-25 | 7 done: interrupts & NVIC | nvic.c — the model's two halves live: PendSV pended via SCB ICSR (IPSR=14) vs USART3 via NVIC ISPR1 (IPSR=55=16+39); pend-while-disabled survives and fires on enable (s128). Finding: ICSR VECTPENDING reads 0 for a pended-but-disabled line — it reports enabled ones only; ISPR is the honest witness. IPR write 0xFF reads 0xF0 → ST's 16 levels. TIM2-vs-I2C1 (s154, both software-pended): equals tail-chain after exit; 0x70 preempts 0x80 mid-handler between two prints; AIRCR PRIGROUP=7 turns the whole byte into sub-priority and the preemption vanishes; simultaneous equal pend → lower IRQ number first. SHCSR fault enables armed (0x70000) for section 8. PendSV_Handler claimed by nvic.c until the scheduler takes it. **Section 7 complete** |
| 2026-08-25 | 7 addendum: SHPR (system-exception priorities) | nvic.c demo 8 + round S — SHPR3 boots 0x0 (all system exceptions at max programmable urgency, why demo 1's PendSV fired instantly); 0xFF→0xF0 same 4-bit rule as IPR; PendSV demoted to 0xF0 then pended from inside TIM2@0x80 waits and tail-chains after — the RTOS offload pattern live. Reset/NMI/HardFault fixed at -3/-2/-1, no register. Closes the "priority registers for system exceptions" syllabus line |
| 2026-08-25 | 7 companion: CMSIS dialect | cmsisnvic.c — same NVIC via core_cm4.h on TIM3/IRQ29: NVIC_SetPriority(5) ↔ raw byte 0x50 (the <<4 dialect trap behind FreeRTOS priority bugs); IPR proven byte-addressable live (IP[29]=0xA0 flips one byte of 0x00005000, no RMW window — why CMSIS beats the slides' word math); pend/enable via CMSIS fires TIM3 (IPSR=45); negative IRQn routes to SCB->SHP (SysTick → SHPR3=0xF0000000); SetPriorityGrouping wraps round C's VECTKEY dance. Rule of the lesson: write CMSIS, think registers |
| 2026-08-26 | 8 done: exception entry/exit & faults | faults.c + faulthandler.c grown into the real decoder (naked TST LR,#4 prologue, named-bit CFSR decode, frame dump — shared via faulthandler.h). s186 all three: undefined instr from SRAM (UNDEFINSTR, stacked pc = the 0xFFFFFFFF array), div-by-zero (quiet 0 untrapped; CCR.DIV_0_TRP → DIVBYZERO, SDIV skipped), exec from 0x4000_0000 (IACCVIOL; MMARVALID=0 — instruction-side violations leave MMFAR silent, the stacked pc is the witness). Recovery by EDITING the stacked frame: pc := lr for doomed calls, pc += 2/4 for skippable opcodes — one run, five faults, still standing. Bus faults: Finding — the L476 quietly reads 0 at both FMC space (0x6000_0000) and un-clocked TIM2, no error; past-SRAM1 0x2001_8000 errors properly: load → PRECISERR + BFAR, store → IMPRECISERR with stacked pc 8 bytes PAST the store and BFARVALID=0 (the lying crash log, live). Escalation: USGFAULTENA off → HardFault, HFSR FORCED with DIVBYZERO still underneath, parked. Finding: EXC_RETURN=0xFFFFFFE9 on every fault — hard-float FPCA set → extended FPU frame (the slides' no-FPU 0xFFFFFFF9 story, upgraded). **Section 8 complete** |
| 2026-08-26 | 8 addendum: GDB fault autopsy | The instructor's debugger walkthrough, done live on demo 7's escalated div-by-zero: breakpoint on the naked HardFault_Handler (batch GDB per docs/cli-workflow.md §5), caught before any instruction ran — LR register = 0xFFFFFFE9 (EXC_RETURN really rides in there), xPSR=0x010F0003 (IPSR=3, HardFault active) vs stacked xpsr 0x010F0000 (IPSR=0, was thread mode), MSP = 0x2001_7F68 = exactly the frame address every serial run printed. Frame words 2/3 = 0xA/0x0 — dividend and divisor caught in the act; x/i on frame word 6 disassembles `sdiv r3, r2, r3`. Finding: FPCCR=0xC0000079, LSPACT=1 — LAZY FP stacking caught mid-act: the 26-word frame's S0-S15 space reserved but never filled (stale stack garbage there). And `bt` walks #1 <signal handler called> → playing_with_faults → main: GDB unwinds via EXC_RETURN + the frame — the by-hand walk, automated |
| 2026-08-26 | 9 done: SVC & PendSV | svc.c — SVC #25: opcode 0xDF19 dug from stacked pc-2, number+4=29 planted in the frame's r0 and delivered by unstacking (s210); calculator via service numbers 36-39 with operands in stacked r0/r1 (s211: -50/-125/332860/-22), wrappers written in the register-pinned idiom real syscall shims use (the slides' MOV-after-SVC works only by -O0 luck). Slide 207's "uncommon" SHCSR SVCALLPENDED trigger proven broken by design: opcode behind the stacked pc = 0x4817, not 0xDFxx — no instruction, no number. s185 closed: SVC #100 inside the SVC handler → HFSR FORCED with stacked xpsr IPSR=11 and EXC_RETURN 0xFFFFFFF1 "handler mode" — the victim was the handler itself. Finding: EXC_RETURN bit 4 is per-CONTEXT, not per-build — SVC #25 fired before any %-format printf had run (no-arg printf folds to puts) → basic 0xFFFFFFF9, while faults.c's first fault came after %-printfs (the printf engine sets FPCA) → extended 0xFFFFFFE9; handler contexts restart FPCA=0 → demo 4's 0xFFFFFFF1. PendSV half pre-covered live by nvic.c round S (demoted PendSV pended from TIM2 tail-chains after = the offload pattern); its context-switch job is section 10's capstone. **Section 9 complete** |
| 2026-08-30 | 10 done: task scheduler capstone | scheduler.c — 4 tasks + idle on 2 KB .bss stacks, SysTick 1 ms (RELOAD 3999 from the 4 MHz MSI vs the course's 16000), PendSV as the switch, forged 17-word initial contexts (argument in r0 like a real RTOS), TCBs, task_delay blocking, WFI idle; PendSV + SysTick demoted to 0xF0. Pins PA5 (LD2/D13), PA6 (D12), PA7 (D11), PA9 (D8) = the 4-LED substitute; exactly one printer (task 1) so newlib printf is never entered twice. Hardware-verified: blocking mode on→on = 2013 ticks (2000 + the 13 ms report), idle loops = 2000 per period; spin mode 8612 ticks per 2 s of CPU with idle = 0 — the ×4 stretch, and the ~9 % above 4×980 is what 1000 switches/s cost at 4 MHz -O0 (~90 µs each: extended-frame tasks push/pop 42 FPU words). Findings: the slides' 0xFFFFFFFD in the frame's lr slot is the task's own r14, not EXC_RETURN; EXC_RETURN is per-task on a hard-float build (T1's FPCA 0→1 after its first printf, T2's saved EXC_RETURN 0xFFFFFFFD→0xFFFFFFED after one float multiply, T3/T4 stay basic) so PendSV saves {r4-r11, LR} + s16-s31 per task — the FreeRTOS CM4F shape, not the slides' R4-R11 swap; the slides start SysTick before the PSP switch (a 1 ms window with PSP = 0) — here the tick only counts until sched_started. nvic.c's PendSV_Handler became nvic_pendsv_demo; the scheduler's naked handler forwards to it until it starts. **Section 10 complete** |
| 2026-08-30 | 11 done: bare-metal build with GNU tools | gnu-build/ — hand Makefile (every gcc/ld command visible; `stages` = main.i/main.s/main.o, `analyze` = objdump/readelf/nm/map), startup.c (99-word `vectors[]` in `.isr_vector`, 91 weak aliases of a reporting Default_Handler, Reset_Handler in C: .data copy, .bss zero, SystemInit, __libc_init_array, main), stm32l476rg.ld from scratch (FLASH/SRAM1/SRAM2 MEMORY, .text with KEEP(.isr_vector), the three *_array tables and .ARM.extab/.exidx newlib asks for, .data `> SRAM1 AT> FLASH` with _sidata = LOADADDR, .bss NOLOAD, _end, ASSERT on SRAM1 overflow), openocd.cfg for CubeIDE's bundled OpenOCD. Verified on the ELF: entry = Reset_Handler|1 (0x0800049d), word 0 at 0x08000000 = 0x20018000, word 3 = faulthandler.c's strong HardFault_Handler (the weak-alias override works), .data segment VirtAddr 0x20000000 / PhysAddr 0x08009fdc — the two addresses of the slide, in `readelf -l`; main.o shows exactly .text/.data/.bss/.rodata at address 0 with R_ARM_THM_CALL relocations for printf/puts. Findings: `-nostartfiles` drops crti.o/crtn.o, so `__libc_init_array` → "undefined reference to _init" (reproduced; empty _init/_fini stubs in startup.c); .data is 1.7 KB before the first user variable (newlib's __malloc_av_ 1032 B + _impure_data); `--orphan-handling=warn` showed libc's .ARM.extab needed a home. Board-verified the same evening after the probe came back (see next row): serial shows the map (.text 0x08000000..0x08009fd4, .data load 0x08009fdc → run 0x20000000..0x200006bc, .bss to 0x2000095c, _end 0x20000960, MSP 0x20017fb0 under _estack 0x20018000), the variable table (global/static in .data at 0x20000000/04, .bss at 0x200006bc, rodata in FLASH, a local at 0x20017fcc), "42 in two places" (0x20000000 and its FLASH image at 0x08009fdc), vector words [0]=0x20018000 [1]=0x0800049d, then the pended IRQ 5 lands in Default_Handler with IPSR=21. The course's path proven too: `make openocd` (bundled OpenOCD: DPIDR, "Cortex-M4 r0p1 processor detected", GDB on :3333) + `make load` (GDB `load` 42648 bytes at 23 KB/s, `monitor reset run`) → same serial output. **Section 11 complete** |
| 2026-08-30 | Bench: the SWD outage was the ST-LINK firmware | Every ST connect mode failed "Unable to get core ID" while NRST/VCP/Vtarget were fine; OpenOCD narrowed it (DPIDR read OK, every AP access → `unknown/unexpected STLINK status code 0x5`, firmware V2J28M18). USB reset did nothing; the headless upgrader did: `/opt/st/stm32cubeclt_1.22.0/STLinkUpgrade.sh` (= bundled JRE + `STLinkUpgrade.jar -update`) took it to V2J48M35 and SWD returned at once (Device ID 0x415). Lesson: when ST's tool fails at "core ID", run OpenOCD with `-d3` and read the STLINK status code before touching the wiring |
| 2026-08-30 | 12 done: OpenOCD, newlib & semihosting — **COURSE COMPLETE** | gnu-build/ grew three links of the same program + two scripted OpenOCD runs. `make compare` (text/data/bss): full newlib 42336/1724/676; newlib-nano 13600/104/608 — `_vfiprintf_r` + `_printf_i` replace `_vfprintf_r` + `_dtoa_r`, nano malloc drops the 1032-byte `__malloc_av_` bins and the big `_reent`; gc (`-ffunction-sections -fdata-sections` + `--gc-sections`) 37248/1724/644 — the bait `unused_helper`/`unused_table` plus uart2_poll/getc and 7 unused syscall stubs listed under "Discarded input sections", `vectors` kept by KEEP; semihosting (`--specs=rdimon.specs`, syscall.c + uart2.c out) 44264/1736/848. Findings: librdimon's stubs are WEAK (`W _write`), so ST's sysmem.c `_sbrk` coexists with it — why CubeIDE only excludes syscalls.c; newlib's own `_sbrk` wants the symbol `end` (script now `PROVIDE(end = .)` beside `_end`); a non-empty `.init_array` is a writable section in FLASH — without `(READONLY)` the linker merges it into the code segment and warns "LOAD segment with RWX permissions" (ST's script has the keyword for this). Board: baseline prints demo 6 `constructor_mark = 0xC0FFEE` (the .init_array walk is real); `make inspect` reads the vector words through the AHB-AP, plants 0xDEADBEEF at 0x20000000, and reads 0x2a back after Reset_Handler's copy loop; `make run-semi` with semihosting OFF halts at pc 0x080091fe = `bkpt 0xab` inside `initialise_monitor_handles` (before any printf — the field crash of a release build with semihosting left on), with it ON the entire lesson arrives on OpenOCD's console. **Section 12 complete — course complete** |
