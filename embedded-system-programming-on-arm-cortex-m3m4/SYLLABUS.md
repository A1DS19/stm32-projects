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
| 4 user LEDs (PD12–PD15) | 1 user LED (LD2, PA5) — scheduler lessons need 3 external LEDs on the breadboard, or printf task markers (decide when we get there) |
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
- [ ] **9. SVC & PendSV** (203–220) — supervisor calls, extracting the SVC number from
  the stacked PC, PendSV as the context-switch workhorse, interrupt offloading.
  *Exercise (s211): SVC handler prints its number, returns number+4.*
  *Exercise (s212): 4-operation calculator where the SVC number picks the operation.*
- [ ] **10. Capstone: task scheduler** (221–276) — the big one. Four user tasks with
  their own stacks, round-robin switching via SysTick + PendSV, dummy initial stack
  frames, task control blocks, a blocking (delay) state, an idle task, a global tick
  count. This is a hand-rolled miniature of what FreeRTOS does — the course's payoff.
  *L476RG note: this is the 4-LED lesson block — plan the LED substitute before starting.*
- [ ] **11. Bare-metal build with GNU tools** (277–344) — cross-compilation without an
  IDE: compiler options, analyzing `.o` files with objdump, sections (.text/.data/.bss),
  writing a startup file from scratch (vector table via attributes, weak/alias
  handlers, copying .data to RAM, zeroing .bss), writing the linker script (MEMORY,
  SECTIONS, location counter, linker symbols, ALIGN).
  *Our own `startup_stm32l476xx.S` + `stm32l476xg_flash.ld` finally get demystified —
  the lesson versions can live alongside and replace them in the build for a lesson.*
- [ ] **12. OpenOCD, newlib & semihosting** (345–378) — downloading/debugging with
  OpenOCD, newlib vs newlib-nano, the low-level syscalls printf needs, `--gc-sections`,
  semihosting, `__libc_init_array`.
  *Toolchain note: we flash with `STM32_Programmer_CLI` today; install OpenOCD when
  this section starts to follow along.*

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
