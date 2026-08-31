/* stm32l476rg.ld — a linker script written from scratch (slides 321-333).
 *
 * The compiler turns each .c file into a relocatable object (.o) whose
 * sections all start at address 0. This file tells the linker three
 * things: which memories the chip has (MEMORY), how to merge the input
 * sections of every .o into output sections and which memory each one
 * goes to (SECTIONS), and the names of a few addresses the startup code
 * needs (symbols). Written in the GNU linker command language; handed
 * to gcc with -T. Compare with ST's ../stm32l476xg_flash.ld: same job,
 * ten times longer because it also serves C++, TLS and SRAM2. */

/* ENTRY: only the ELF header's "entry point" field. The core itself
 * ignores it — it boots from vector[1] (resetseq.c). Debuggers and
 * OpenOCD read it to know where the program starts. */
ENTRY(Reset_Handler)

/* MEMORY: name, allowed contents (r read, w write, x execute), start,
 * size. The linker places sections with these and refuses to build if
 * something does not fit. Sizes from the STM32L476RG datasheet. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    SRAM1 (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
    SRAM2 (rwx) : ORIGIN = 0x10000000, LENGTH = 32K   /* declared, unused here */
}

/* Symbols: a linker symbol is a NAME FOR AN ADDRESS, not a variable.
 * C code reaches them with `extern uint32_t _estack;` and must always
 * take the address (&_estack) — never read the "value". */
_estack = ORIGIN(SRAM1) + LENGTH(SRAM1);   /* vector[0]: the initial stack pointer */
_Min_Stack_Size = 0x400;                   /* sysmem.c keeps the heap this far below _estack */
_Min_Heap_Size = 0x200;

SECTIONS
{
    /* Output section .text goes to FLASH. Inside the braces, each line
     * names input sections to merge: `*(.text)` = the .text section of
     * every input file. `.` is the location counter — the address being
     * filled right now; the linker moves it as it places things. */
    .text :
    {
        KEEP(*(.isr_vector))    /* the vector table FIRST, at 0x08000000. Nothing
                                 * references it by name, so KEEP protects it if
                                 * --gc-sections is ever switched on */
        *(.text)                /* code of every .o */
        *(.text.*)              /* code the compiler put in named subsections (libc does) */
        *(.rodata)              /* const data and string literals */
        *(.rodata.*)
        . = ALIGN(4);           /* round the location counter up to a word */
        _etext = .;             /* name for "end of code" */
    } > FLASH

    /* newlib's __libc_init_array walks three constructor tables using
     * these six symbols. C has no constructors, so the tables are empty —
     * but the symbols must exist or the link fails. KEEP for the same
     * reason as the vector table. */
    .preinit_array :
    {
        __preinit_array_start = .;
        KEEP(*(.preinit_array*))
        __preinit_array_end = .;
    } > FLASH
    .init_array :
    {
        __init_array_start = .;
        KEEP(*(.init_array*))
        __init_array_end = .;
    } > FLASH
    .fini_array :
    {
        __fini_array_start = .;
        KEEP(*(.fini_array*))
        __fini_array_end = .;
    } > FLASH

    /* Stack-unwinding tables: C++ exceptions need them, C does not, but
     * a few libc objects (strlen, memcpy...) carry some. Give them a home
     * so the linker does not invent an "orphan" section on its own —
     * link with -Wl,--orphan-handling=warn to see what else it would
     * invent (only empty linker-stub sections like .glue_7 remain). */
    .ARM.extab :
    {
        *(.ARM.extab*)
    } > FLASH
    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* .data: initialized globals. Two addresses per byte! They RUN in
     * SRAM1 (`> SRAM1`, the VMA) but their initial values are STORED in
     * FLASH right after .text (`AT> FLASH`, the LMA) — RAM is empty at
     * power-up. Reset_Handler copies LMA -> VMA before main. */
    _sidata = LOADADDR(.data);  /* where the FLASH copy starts */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;             /* run-time start */
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;             /* run-time end */
    } > SRAM1 AT> FLASH

    /* .bss: uninitialized globals. NOLOAD: no bytes in the file, no
     * FLASH spent — only a range of SRAM1 that Reset_Handler zeroes.
     * COMMON = old-style tentative definitions, same treatment. */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        . = ALIGN(8);
        _end = .;               /* the heap grows up from here (sysmem.c's _sbrk) */
    } > SRAM1

    /* Refuse to link if data + bss + heap + stack cannot fit. */
    ASSERT(_end + _Min_Heap_Size + _Min_Stack_Size <= _estack,
           "SRAM1 overflow: .data + .bss + heap + stack do not fit in 96 KB")
}
