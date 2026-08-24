# Where we left off
_2026-08-24 (evening — Cortex-M course, sections 2-3 done)_

## This session (2026-08-23 → 24)
- **Cortex-M3/M4 course started** (step 2 of the FastBit path):
  `embedded-system-programming-on-arm-cortex-m3m4/` scaffolded by cloning the sibling's
  shape (decision recorded: no regeneration, keeps the FPU/printf + serial-by-identity
  fixes). The 375-slide deck lives distilled in its `SYLLABUS.md` (12 sections, slide
  ranges, exercises, F407→L476RG adaptations); `slides.pdf` + instructor repo clone
  `course-code/` (niekiran/CortexMxProgramming, 16 CubeIDE projects mapped to sections)
  are in the project but gitignored — paid/foreign material.
- **Six lessons done, each hardware-verified over the VCP** (pattern: one
  `Src/<lesson>.c` + `Inc/<lesson>.h`, `main.c` is a thin dispatcher, git = archive):
  1. `modes.c` — thread vs handler, STIR-pended IRQ 3, IPSR printed 0→19→0
  2. `access.c` — PAL/nPAL via CONTROL.nPRIV, same NVIC touch works→hard-faults;
     finding: CONTROL=5 (FPCA set by hard-float printf)
  3. `coreregs.c` — MOV/MRS snapshot vs pointer reads (CPUID/ISER0/AHB2ENR);
     findings: MRS reads EPSR slice (T bit) as 0; MSP snapshot one call-frame below SP
  4. `inlineasm.c` — operand contract, s39 load/add/store exercise on &var addresses,
     MRS/MSR PRIMASK 0→1→0
  5. `resetseq.c` — vector[0]==&_estack, vector[1]==&Reset_Handler, boot alias @0;
     finding: VTOR still 0, ISRs fetched via alias (bootloader course will move it)
  6. `tbit.c` — bit0 cleared on a real function → INVSTATE, CFSR=0x00020000
  `faulthandler.c` extracted as the single strong HardFault_Handler (prints CFSR
  decoded + the handler-mode-privilege proof); lessons borrow distinct IRQs (modes=3
  RTC_WKUP, access=4 FLASH) because all lesson files compile together.
- **Tooling**: user's generate-c-cpp-project conventions folded into the
  `stm32-new-project` skill AND backported here — `.clang-format` (4-space, LLVM,
  pointer-left), `make format`, clang-tidy `.clangd`. Live-fire exemptions added both
  places (test-first, suite 12/12): sp/pc/lr + loop counters vs identifier-length;
  linker symbols (`_estack`…) vs naming + cert-dcl37/51 + bugprone-reserved-identifier.
  Skill also now pins launch/tasks to each project's own `.vscode/` (F5 broke when the
  project folder was opened as VS Code root — `'<your program>' does not exist`).

## State
- `stm32-projects` main at `2a4b293`, pushed, clean except long-standing strays
  (user committed the docs PDFs + sibling `.vscode`/`.settings` themselves in d137868).
- `claude-configs` main at `4faca0d`, pushed; `settings.json` dirty there — not ours.
- **Board is parked in HardFault_Handler on purpose** (t-bit lesson ends crashed) —
  `make reset` or black RESET before expecting serial output.
- Serial-verify recipe used all session: background `timeout 3 cat $port` + `make reset`,
  then read the log (port = the ttyACM with `ID_MODEL=STM32_STLink`).

## Next session
1. Next lesson: **memory map & bus interfaces** (slides 54–69; SYLLABUS section 4) —
   mostly map theory; a probe-the-regions demo fits (CODE/SRAM/peripheral/PPB reads).
2. Then **bit-banding** (70–78) with exercise s76 — it pokes 0x2000_0200 raw: check
   what our .data/.bss occupy first (same stomping concern the inline-asm lesson dodged
   with &var addresses).
3. Then the big **stack section** (79–117): MSP/PSP banking, CONTROL.SPSEL — the
   PSP=0 and MSP-frame-gap breadcrumbs from coreregs.c pay off there.
4. Keep the SYLLABUS progress table + section checkboxes current per lesson;
   `course-code/Source_code/` folder map in SYLLABUS names each answer key.
