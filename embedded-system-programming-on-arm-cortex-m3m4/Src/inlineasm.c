/* Lesson: ARM GCC inline assembly (slides 34-48, instructor's 004_inline_1).
 * This pays off the IOU on every MRS line so far.
 *
 * General form:
 *     __asm volatile("template" : outputs : inputs : clobbers);
 *
 * The template is real assembly with %0, %1, ... holes, numbered in the
 * order operands appear (outputs first). The three lists after it are the
 * CONTRACT with the compiler — it owns every register, so you must declare
 * everything you touch:
 *   outputs  "=r"(cvar) — give me a scratch register; whatever the
 *            template leaves in it lands in cvar afterwards (= means
 *            write-only)
 *   inputs   "r"(cvar)  — put this C value in some register for me
 *   clobbers "r0", "memory" — things the template changed behind the
 *            compiler's back: named registers, or RAM ("memory") when you
 *            store through a pointer
 * `volatile` on the asm means: don't delete or reorder this even if the
 * results look unused — same instinct as volatile on a register pointer.
 *
 * Steps below: (1) pure-operand ADD, no clobbers needed; (2) the slide-39
 * exercise — load two values FROM MEMORY, add, store back — with LDR/STR
 * and a clobber list; (3) the MRS/MSR pair, toggling PRIMASK live.
 *
 * One deliberate change from the instructor: his memory exercise pokes raw
 * addresses like 0x2000_1000. On our running program that block of SRAM
 * belongs to the stack and our variables (memory-map lesson!), so we take
 * addresses of C variables instead — same LDR/STR mechanics, nothing
 * stomped. */

#include "inlineasm.h"

#include "uart2.h"

#include <stdint.h>
#include <stdio.h>

void playing_with_inline_assembly(void) {
    uart2_init();

    printf("\r\nLesson: inline assembly\r\n");

    /* 1 — operands only. The compiler picks all three registers; we never
     * name one, so there is nothing to clobber. */
    uint32_t lhs = 12;
    uint32_t rhs = 30;
    uint32_t sum;
    __asm volatile("ADD %0, %1, %2" : "=r"(sum) : "r"(lhs), "r"(rhs));
    printf("ADD %%0,%%1,%%2       : %lu + %lu = %lu\r\n", (unsigned long)lhs, (unsigned long)rhs,
           (unsigned long)sum);

    /* 2 — slide-39 exercise: values live in MEMORY, so the template must
     * load them, add, and store the result back. We name R0/R1 ourselves
     * and write RAM, so both go in the clobber list. Inputs are the
     * ADDRESSES, handed over in registers. */
    uint32_t first = 1050;
    uint32_t second = 926;
    uint32_t result = 0;
    __asm volatile("LDR R0, [%0]\n\t"
                   "LDR R1, [%1]\n\t"
                   "ADD R0, R0, R1\n\t"
                   "STR R0, [%2]"
                   :
                   : "r"(&first), "r"(&second), "r"(&result)
                   : "r0", "r1", "memory");
    printf("LDR/LDR/ADD/STR    : mem %lu + mem %lu -> mem %lu\r\n", (unsigned long)first,
           (unsigned long)second, (unsigned long)result);

    /* 3 — special registers, the MRS/MSR pair. PRIMASK is the safe demo:
     * bit 0 set masks every interrupt with configurable priority. Watch
     * it go 0 -> 1 -> 0. (CPSID i / CPSIE i are shorthand instructions
     * for exactly this — the NVIC section uses them.) */
    uint32_t primask;
    __asm volatile("MRS %0, PRIMASK" : "=r"(primask));
    printf("MRS PRIMASK        : %lu (interrupts open)\r\n", (unsigned long)primask);

    __asm volatile("MSR PRIMASK, %0" : : "r"(1U));
    __asm volatile("MRS %0, PRIMASK" : "=r"(primask));
    printf("MSR PRIMASK, 1     : %lu (all maskable interrupts held off)\r\n",
           (unsigned long)primask);

    __asm volatile("MSR PRIMASK, %0" : : "r"(0U));
    __asm volatile("MRS %0, PRIMASK" : "=r"(primask));
    printf("MSR PRIMASK, 0     : %lu (open again)\r\n", (unsigned long)primask);
}
