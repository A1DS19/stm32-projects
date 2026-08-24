# Course map — Embedded System Programming on ARM Cortex-M3/M4 (FastBit)

Distilled from the course slide deck (`slides.pdf`, 375 slides, 2024 edition — kept out
of git, it's paid course material). Check sections off as they land; each lesson's code
goes in `Src/` and git history is the archive, same as the last course.

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
- [ ] **3. Reset sequence** (49–53) — what the core does from power-up to `main`:
  MSP fetch from address 0, reset handler, why the vector table starts with an SP value.
- [ ] **4. Memory map & buses** (54–69) — the fixed 4 GB map (CODE/SRAM/peripheral/PPB
  regions), AHB vs APB, the chip block diagram.
- [ ] **5. Bit-banding** (70–78) — alias addresses that turn one bit into one word;
  atomic bit writes without read-modify-write.
  *Exercise (s76): write 0xFF to 0x2000_0200, clear bit 7 both ways, compare.*
- [ ] **6. Stack memory** (79–117) — MSP vs PSP, the four stack models (we use
  full-descending), switching thread mode to PSP, AAPCS (which registers a C function
  may trash, how arguments travel), stacking/unstacking on interrupt.
  *Exercise: instruction-level debugging of stack activity; change SP to PSP.*
- [ ] **7. Interrupts & the NVIC** (118–157) — SCB, enabling faults and PendSV, the
  steps to wire any MCU peripheral interrupt, priority values vs urgency, priority
  grouping (pre-empt vs sub-priority), the 16 priority levels ST implements.
  *Exercise (s128): enable and pend the USART3 interrupt from software.*
  *Exercise (s154): interrupt priority configuration.*
- [ ] **8. Exception entry/exit & faults** (158–202) — the exception stack frame,
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
