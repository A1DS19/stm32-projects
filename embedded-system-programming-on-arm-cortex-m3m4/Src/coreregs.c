/* Lesson: core registers, parts 1-3 + memory-mapped vs non-memory-mapped
 * registers (slides 32-33).
 *
 * Every register in this MCU falls on one side of a line:
 *
 *   NON-memory-mapped — the processor's own registers. No address in the
 *   4 GB map, so no C pointer can ever reach them; only register-name
 *   instructions can (MOV for the ordinary ones, MRS/MSR for the special
 *   ones — the inline assembly lesson makes these official):
 *     R0-R12 — general purpose, where the CPU actually computes
 *     R13    — SP, the stack pointer. Really TWO banked registers: MSP
 *              (main, everyone's default) and PSP (process — parked until
 *              the stack lesson flips CONTROL.SPSEL)
 *     R14    — LR, link register: where to return to after a call
 *     R15    — PC, program counter: the address being executed
 *     PSR    — status (the xPSR we decoded: flags, T-bit, IPSR)
 *     PRIMASK/FAULTMASK/BASEPRI — interrupt mask registers (NVIC section)
 *     CONTROL — nPRIV/SPSEL/FPCA (last lesson's register)
 *
 *   Memory-mapped — everything else, processor peripherals (NVIC, SCB,
 *   SysTick in the 0xE000_xxxx System Control Space) and MCU peripherals
 *   (RCC, GPIO, USART...) alike: each register IS an address in the map,
 *   and a plain volatile C pointer reads it. Where the address lands
 *   previews the memory-map lesson: code in flash at 0x0800_xxxx, the
 *   stack in SRAM at 0x2000_xxxx, MCU peripherals at 0x4000_xxxx, the
 *   core's own block at 0xE000_xxxx.
 *
 * The demo snapshots the special registers BEFORE any printf, prints the
 * lot, then reads three memory-mapped registers by bare address. Last
 * line re-reads CONTROL: printf's FPU use has set FPCA by then (bit 2 —
 * last lesson's surprise), so 0 becomes 4 between snapshot and reprint. */

#include "coreregs.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

/* MRS = "move from special register" — the only road to the named-only
 * registers. The register name must sit inside the instruction itself, so
 * one tiny function per name; the macro only writes them for us. */
#define DEFINE_SPECIAL_READER(fn, reg)                                                             \
    static uint32_t fn(void) {                                                                     \
        uint32_t value;                                                                            \
        __asm volatile("MRS %0, " reg : "=r"(value));                                              \
        return value;                                                                              \
    }

DEFINE_SPECIAL_READER(read_msp, "MSP")
DEFINE_SPECIAL_READER(read_psp, "PSP")
DEFINE_SPECIAL_READER(read_xpsr, "XPSR")
DEFINE_SPECIAL_READER(read_primask, "PRIMASK")
DEFINE_SPECIAL_READER(read_faultmask, "FAULTMASK")
DEFINE_SPECIAL_READER(read_basepri, "BASEPRI")
DEFINE_SPECIAL_READER(read_control, "CONTROL")

void playing_with_core_registers(void) {
    uint32_t lr;
    uint32_t pc;
    uint32_t sp;

    uart2_init();

    /* ordinary registers read with plain MOV; LR right now holds the
     * return path of the uart2_init call — a code address, like PC */
    __asm volatile("MOV %0, LR" : "=r"(lr));
    __asm volatile("MOV %0, PC" : "=r"(pc));
    __asm volatile("MOV %0, SP" : "=r"(sp));

    uint32_t msp = read_msp();
    uint32_t psp = read_psp();
    uint32_t xpsr = read_xpsr();
    uint32_t primask = read_primask();
    uint32_t faultmask = read_faultmask();
    uint32_t basepri = read_basepri();
    uint32_t control = read_control();

    printf("\r\nLesson: core registers — no address, no pointer, MRS/MOV only\r\n");
    printf("PC   = 0x%08lx  code region: flash, where execution is\r\n", (unsigned long)pc);
    printf("LR   = 0x%08lx  also flash: the pending return address\r\n", (unsigned long)lr);
    printf("SP   = 0x%08lx  SRAM: the stack, near the top of RAM\r\n", (unsigned long)sp);
    printf("MSP  = 0x%08lx  SP minus read_msp()'s own frame: same register,\r\n"
           "                  snapshotted one call deeper — stacks move per call\r\n",
           (unsigned long)msp);
    printf("PSP  = 0x%08lx  parked until the stack lesson banks SP\r\n", (unsigned long)psp);
    printf("xPSR = 0x%08lx  reads 0 here! MRS gets the EPSR slice (T-bit) as\r\n"
           "                  zero by design — only the debugger sees the 1\r\n",
           (unsigned long)xpsr);
    printf("PRIMASK=%lu FAULTMASK=%lu BASEPRI=%lu  no interrupts masked\r\n",
           (unsigned long)primask, (unsigned long)faultmask, (unsigned long)basepri);
    printf("CONTROL=%lu at snapshot: privileged, MSP, no FP context yet\r\n",
           (unsigned long)control);

    printf("\r\nmemory-mapped — same 4GB map, plain C pointers:\r\n");

    /* SCB CPUID: the core introduces itself. [15:4] = part number,
     * 0xC24 = Cortex-M4; [3:0] = revision */
    volatile uint32_t* cpuid = (uint32_t*)0xE000ED00;
    printf("SCB CPUID  @0xE000ED00 = 0x%08lx  part=0x%03lx (0xC24 = Cortex-M4) r0p%lu\r\n",
           (unsigned long)*cpuid, (unsigned long)((*cpuid >> 4) & 0xFFFU),
           (unsigned long)(*cpuid & 0xFU));

    /* NVIC ISER0: fresh reset, nothing unmasked this run */
    volatile uint32_t* iser0 = (uint32_t*)0xE000E100;
    printf("NVIC ISER0 @0xE000E100 = 0x%08lx  no IRQs unmasked this run\r\n",
           (unsigned long)*iser0);

    /* RCC AHB2ENR, an MCU peripheral register: bit 0 (GPIOAEN) is on
     * because uart2_init clocked GPIOA for the PA2/PA3 pins */
    volatile uint32_t* ahb2enr = (uint32_t*)0x4002104C;
    printf("RCC AHB2ENR@0x4002104C = 0x%08lx  GPIOAEN set by uart2_init\r\n",
           (unsigned long)*ahb2enr);

    printf("\r\nCONTROL now = %lu — printf's FPU use set FPCA meanwhile\r\n",
           (unsigned long)read_control());
}
