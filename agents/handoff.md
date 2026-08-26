# Where we left off
_2026-08-25 (evening — Cortex-M course, section 7 complete in three commits)_

## This session (2026-08-25)
- **Section 7 — interrupts & the NVIC — done and hardware-verified**, three commits:
  1. `nvic.c` (9576344): the exception model live — PendSV via SCB ICSR (IPSR=14) vs
     USART3 via NVIC ISPR1 (IPSR=55); pend-while-disabled fires on enable (s128);
     IPR 0xFF→0xF0 = 16 levels; s154 as rounds A/B/C/T (equals tail-chain, 0x70
     preempts 0x80 mid-handler, PRIGROUP=7 kills preemption, tie → lower IRQ number).
     Finding: ICSR VECTPENDING reads 0 for a pended-but-DISABLED line — enabled-only;
     ISPR is the honest witness. SHCSR fault enables armed (0x70000) but per-boot.
  2. SHPR addendum (2151331): demo 8 + round S — SHPR3 boots 0x0; PendSV demoted to
     0xF0 then pended from inside TIM2@0x80 waits and tail-chains after = the RTOS
     offload pattern live. Reset/NMI/HardFault fixed −3/−2/−1.
  3. `cmsisnvic.c` (10e4e62): same NVIC through CMSIS on TIM3/IRQ29 — 5↔0x50 dialect
     (the FreeRTOS priority-config trap), IPR proven byte-addressable (IP[29]=0xA0
     single STRB flips one byte of 0x00005000, no RMW race — CMSIS beats the slides'
     word math), negative IRQn routes to SCB->SHP (CM4 has byte array SHP[12], no
     SHPR3 member — cast &SHP[8]), SetPriorityGrouping = round C's VECTKEY dance.
     Lesson rule: write CMSIS, think registers.
- Long conceptual Q&A arc between lessons (EXTI chain pin→SYSCFG→EXTI→NVIC, AAPCS,
  Linux-driver comparison, CAN/DMA, embedded-career map) — no repo impact.
- Decided (memory: capstone-after-roadmap): no side projects until the course roadmap
  is done; front-runner after = point-to-point LoRa messenger.

## State
- `stm32-projects` main at `10e4e62`, pushed. Only stray: `.clangd` modified by the
  user themselves — left uncommitted, don't fold it into lesson commits.
- Board flashed with `cmsisnvic.c` lesson, parked in the idle loop (no fault, no
  reset gotcha).
- IRQ/handler claims (all lesson files compile together): 3 modes, 4 access,
  6 stackmem, 28/31/39 + PendSV_Handler nvic.c, 29 cmsisnvic.c. PendSV_Handler
  ownership moves to the scheduler lessons when they arrive (single-strong-symbol
  dance like faulthandler.c).
- Serial-verify recipe unchanged: background `timeout 6 cat /dev/ttyACM1`
  (ID_MODEL=STM32_STLink) + `make reset`, then read the log.

## Next session
1. User asked to "continue with interrupt priority and configuration" — **that block
   is fully covered** (nvic.c demos 3–8 + rounds + cmsisnvic.c; see SYLLABUS progress
   table). The actual next material is **section 8: exception entry/exit & faults**
   (slides 158–202; instructor folders 011_exception_entry_exit, 012_fault_gen).
2. Section 8 plan seeds: SHCSR arming is per-boot — the fault lesson arms
   usage/bus/memmanage itself; exercise s186 = cause faults on purpose (undefined
   instruction, div-by-zero via DIV_0_TRP, exec-from-peripheral-region); naked
   handler pulling the stacked PC from the frame; CFSR decoding already half-built
   in faulthandler.c (extend, don't duplicate).
3. Ritual per lesson: "When you'll use this" header + LESSONS.md row + SYLLABUS
   checkbox/progress row + hardware verify + commit (no AI attribution).
