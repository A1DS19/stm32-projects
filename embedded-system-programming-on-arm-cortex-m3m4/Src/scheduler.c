/* Lesson: the task scheduler — the course's capstone (slides 221-276).
 *
 * Everything so far was a part; this is the machine. Four "tasks" (C
 * functions that never return) each get a private stack. A SysTick
 * interrupt fires every 1 ms and pends PendSV. PendSV_Handler is the
 * context switch: it saves the running task's registers on that task's
 * own stack, picks the next task, loads that task's registers from ITS
 * stack, and returns from the exception — into a different function
 * than the one that was interrupted. Each task believes it owns the CPU.
 * That is what an RTOS is; FreeRTOS's port.c for this core is this file
 * with more options.
 *
 * The pieces, each one an earlier lesson:
 *   - tasks run in thread mode on PSP, the scheduler and the two
 *     handlers run on MSP (stackmem.c's floor plan, now with 5 process
 *     stacks instead of 1);
 *   - the 8-word frame the core stacks on exception entry (faults.c,
 *     svc.c) is the top half of a task's saved state. PendSV adds the
 *     other 9 words by hand: r4-r11, and the handler's own EXC_RETURN;
 *   - a task that has never run has no saved state yet, so we FORGE one:
 *     a hand-written frame whose pc is the task's entry function and
 *     whose r0 is the task's argument. The first switch into that task
 *     is an exception return that never had an entry. faults.c edited a
 *     frame to move a program; here we write one from scratch to START
 *     a program;
 *   - PendSV sits at the lowest priority (nvic.c round S), so the switch
 *     always runs after every interrupt has finished, never in the
 *     middle of one.
 *
 * Two builds in one file. SCHED_SPIN_DELAYS is the slides' first
 * version: tasks wait by burning CPU in a loop, so a 1 s wait costs 1 s
 * of CPU and, with four tasks sharing the CPU in 1 ms slices, takes
 * ~4 s of wall time — and the idle task never gets a turn.
 * SCHED_BLOCKING_DELAYS is the finished version: task_delay(n) marks
 * the task BLOCKED until tick n and gives the CPU away at once; the
 * tick handler wakes it later; when every task is blocked the idle task
 * runs (and sleeps the core with WFI). Same scheduler, only the wait
 * call changes — and the numbers in the serial log show the difference.
 *
 * Three places where this lesson departs from the slides, on purpose:
 *
 *   1. The frame's lr slot is the task's OWN r14, not EXC_RETURN. The
 *      instructor writes 0xFFFFFFFD into the stacked lr and calls it
 *      "the value that controls the exception exit". It does not: the
 *      value that controls the exit is the HANDLER's LR (svc.c, faults.c
 *      read it as exc_return). The stacked lr is simply where the task
 *      would return to if its function ever returned — we point it at a
 *      trap. Where 0xFFFFFFFD does belong is the 9th hand-saved word:
 *      see 2.
 *   2. This build is hard-float, and the svc lesson found that
 *      EXC_RETURN bit 4 is per-CONTEXT: a task that has used the FPU
 *      (printf's engine does) is stacked with the 26-word extended
 *      frame (EXC_RETURN 0xFFFFFFED), a task that never has gets the
 *      8-word basic frame (0xFFFFFFFD). The slides' switch keeps the
 *      handler's entry LR and uses it to return into a DIFFERENT task —
 *      wrong frame size, wrong unstacking, the moment one task uses the
 *      FPU and another doesn't. The fix is what every real Cortex-M4F
 *      port does: save EXC_RETURN with each task's context, and save
 *      s16-s31 (the callee-saved FPU registers the core does not stack)
 *      whenever bit 4 says the task has FPU state. The forged context's
 *      EXC_RETURN is 0xFFFFFFFD: thread mode, PSP, basic frame.
 *   3. Arm the switch last. The slides start SysTick before thread mode
 *      has moved to PSP — a 1 ms window in which PendSV would save a
 *      context through PSP = 0. Here the tick only counts until
 *      sched_started is set, and that happens after the PSP switch.
 *
 * Smaller choices: task stacks are plain .bss arrays (the linker knows
 * about every byte, `make size` shows them) instead of addresses carved
 * from the top of RAM by hand; the scheduler keeps the linker's main
 * stack rather than moving MSP under main's feet; 2 KB per task because
 * task 1 calls printf, which alone needs about 1 KB; pins are written
 * through BSRR so four tasks sharing one port never read-modify-write
 * the same register; the configurable faults stay disarmed on purpose —
 * faults.c's handlers "recover" by skipping the faulting instruction,
 * which is the wrong medicine for a broken frame, so a crash escalates
 * to faulthandler.c's parking HardFault with a frame dump instead.
 *
 * When you'll use this: every RTOS port for Cortex-M is this file —
 * FreeRTOS's xPortPendSVHandler, Zephyr's arch_switch, ThreadX's
 * PendSV — and reading them is now reading something you wrote. The
 * bugs you will meet: a task whose stack "mysteriously" overflows the
 * day someone adds a printf (the 1 KB rule); a system that works until
 * one task uses a float (departure 2 — the FPU half of the context,
 * or the port's FPU option left off); a crash at the very first tick
 * (departure 3); an ISR that calls the kernel's yield from above the
 * scheduler's priority (nvic.c's SHPR addendum). And the interview
 * question "what is a context switch?" — answered by the 20 lines of
 * PendSV_Handler below. */

#include "scheduler.h"

#include "faulthandler.h" /* struct FaultFrame: the 8 core-stacked words, third use */
#include "nvic.h"         /* nvic_pendsv_demo: PendSV's job until the scheduler starts */
#include "stm32l476xx.h"  /* GPIOA + RCC for the four task pins */
#include "uart2.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* ---- core registers, raw (CMSIS names in the comments) ---- */

#define SYST_CSR (*(volatile uint32_t*)0xE000E010)  /* SysTick->CTRL */
#define SYST_RVR (*(volatile uint32_t*)0xE000E014)  /* SysTick->LOAD */
#define SYST_CVR (*(volatile uint32_t*)0xE000E018)  /* SysTick->VAL */
#define SCB_ICSR (*(volatile uint32_t*)0xE000ED04)  /* SCB->ICSR */
#define SCB_SHPR3 (*(volatile uint32_t*)0xE000ED20) /* SCB->SHP[10] PendSV, SHP[11] SysTick */

#define SYST_CSR_ENABLE (1UL << 0)
#define SYST_CSR_TICKINT (1UL << 1)
#define SYST_CSR_CLKSOURCE (1UL << 2) /* 1 = count the processor clock (HCLK) */
#define ICSR_PENDSVSET (1UL << 28)
#define SHPR3_PENDSV_LOWEST (0xF0UL << 16)
#define SHPR3_SYSTICK_LOWEST (0xF0UL << 24)

/* The course's F407 boots on a 16 MHz HSI: 16 MHz / 1 kHz = 16000. The
 * L476 boots on the 4 MHz MSI, so 1 ms is 4000 clocks — SysTick counts
 * from RELOAD down to 0 and fires on the 0, so RELOAD = 4000 - 1. (016
 * spells all of this SysTick_Config(4000) through CMSIS.) */
#define CPU_HZ 4000000UL
#define TICK_HZ 1000UL
#define SYSTICK_RELOAD (CPU_HZ / TICK_HZ - 1UL)

/* ---- tasks ---- */

#define TASK_COUNT 5U /* index 0 = idle, 1..4 = the user tasks */
#define USER_TASK_COUNT 4U
#define IDLE_TASK 0U
#define STACK_WORDS 512U /* 2 KB each */

#define EXC_RETURN_THREAD_PSP_BASIC 0xFFFFFFFDUL
#define EXC_RETURN_FPU_STATE_BIT (1UL << 4) /* clear = extended frame, FPU state stacked */
#define XPSR_THUMB (1UL << 24)              /* the T bit lives in xPSR bit 24 */
#define CONTROL_FPCA (1UL << 2)

enum TaskState { TASK_READY, TASK_BLOCKED };

/* Task control block: what the scheduler must remember about a task
 * while it is off the CPU. Everything else — every register — is on the
 * task's own stack, and psp says where. */
struct Tcb {
    uint32_t psp;
    uint32_t wake_tick; /* while BLOCKED: the tick that makes it READY again */
    enum TaskState state;
};

/* A task's saved state as it sits on its stack, lowest address first.
 * Words 0-8 are pushed by PendSV_Handler; words 9-16 are the frame the
 * core pushes by itself on exception entry (faults.c/svc.c's struct —
 * the same eight words, now a task's identity). A task that had FPU
 * state has s16-s31 between the two halves and s0-s15 + FPSCR after the
 * frame; this view still holds, because the FPU words are pushed first
 * and the 9 words below them are always where psp points. */
struct SavedContext {
    uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
    uint32_t exc_return; /* the handler's LR when this task was switched out */
    struct FaultFrame frame;
};

static uint32_t task_stacks[TASK_COUNT][STACK_WORDS] __attribute__((aligned(8)));
static struct Tcb tasks[TASK_COUNT];

static volatile uint32_t current_task = 1U; /* task 1 goes first */
static volatile uint32_t tick_count;
static volatile uint32_t idle_loops;
static enum SchedMode sched_mode;

/* Read by name from PendSV_Handler's assembly — must stay a real symbol. */
volatile uint8_t sched_started;

/* PA5 = LD2 (D13), PA6 = D12, PA7 = D11, PA9 = D8 — all on header CN5, all
 * on one port, none used by the side demos. Task 1 needs no wiring. */
static const uint8_t task_pin[TASK_COUNT] = {0, 5, 6, 7, 9};
static const uint32_t half_period_ms[TASK_COUNT] = {0, 1000, 500, 250, 125};
#define SPIN_PER_MS 300U /* loop turns per ms of CPU at 4 MHz, -O0 (measured at start) */

static uint32_t read_control(void) {
    uint32_t value;
    __asm volatile("MRS %0, CONTROL" : "=r"(value));
    return value;
}

/* ---- the pins ---- */

static void task_pins_init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    for (uint32_t task = 1; task < TASK_COUNT; task++) {
        uint32_t shift = 2U * task_pin[task];
        GPIOA->MODER &= ~(3UL << shift);
        GPIOA->MODER |= 1UL << shift; /* 01 = general-purpose output */
        GPIOA->BRR = 1UL << task_pin[task];
    }
}

/* BSRR: one store sets (low half) or resets (high half) exactly the bits
 * written as 1 — no read-modify-write, so four tasks sharing GPIOA can
 * be interrupted mid-write and still never corrupt each other's pin
 * (the bitband lesson's s76 race, avoided by the hardware). */
static void task_pin_write(uint32_t task, bool on) {
    uint32_t bit = 1UL << task_pin[task];
    GPIOA->BSRR = on ? bit : bit << 16;
}

/* ---- the tick ---- */

static void systick_init(void) {
    SYST_RVR = SYSTICK_RELOAD;
    SYST_CVR = 0; /* any write clears the counter so the first tick is a full one */
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

static void wake_blocked_tasks(void) {
    /* == is enough: this runs on every single tick, so the wake tick can't be missed */
    for (uint32_t task = 1; task < TASK_COUNT; task++) {
        if (tasks[task].state == TASK_BLOCKED && tasks[task].wake_tick == tick_count) {
            tasks[task].state = TASK_READY;
        }
    }
}

void SysTick_Handler(void) {
    tick_count++;
    if (!sched_started) {
        return; /* only a clock until sched_start() arms the switch (departure 3) */
    }
    wake_blocked_tasks();
    SCB_ICSR = ICSR_PENDSVSET; /* write-1-to-set: a plain store, no RMW (nvic.c) */
}

/* ---- the scheduler's decisions (C, called from PendSV's assembly) ---- */

void sched_save_psp(uint32_t psp);
void sched_pick_next(void);
uint32_t sched_load_psp(void);

void sched_save_psp(uint32_t psp) {
    tasks[current_task].psp = psp;
}

uint32_t sched_load_psp(void) {
    return tasks[current_task].psp;
}

/* Round robin over the user tasks, starting just after the current one;
 * the idle task only when nobody else is READY. */
void sched_pick_next(void) {
    uint32_t candidate = current_task;
    for (uint32_t tries = 0; tries < USER_TASK_COUNT; tries++) {
        candidate = candidate % USER_TASK_COUNT + 1U; /* 1→2→3→4→1, idle→1 */
        if (tasks[candidate].state == TASK_READY) {
            current_task = candidate;
            return;
        }
    }
    current_task = IDLE_TASK;
}

/* ---- the context switch ---- */

/* Naked: no compiler prologue may touch a register or the stack before
 * we have saved them. Runs on MSP; PSP is the interrupted task's.
 * Reads sched_started by name so nvic.c's lesson still gets its PendSV
 * print when it is the one running (single strong symbol, shared). */
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile(
        "MOVW  R0, #:lower16:sched_started \n"
        "MOVT  R0, #:upper16:sched_started \n"
        "LDRB  R0, [R0]                    \n"
        "CBNZ  R0, 1f                      \n"
        "B     nvic_pendsv_demo            \n" /* LR still holds EXC_RETURN: it returns for us */
        /* save the outgoing task on its own stack */
        "1: MRS R0, PSP                    \n"
        "TST   LR, #0x10                   \n" /* bit 4 clear: this task has FPU state */
        "IT    EQ                          \n"
        "VSTMDBEQ R0!, {S16-S31}           \n" /* the FPU registers the core does not stack */
        "STMDB R0!, {R4-R11, LR}           \n" /* r4-r11 + THIS task's EXC_RETURN */
        "BL    sched_save_psp              \n"
        /* pick the next one and load it from its stack */
        "BL    sched_pick_next             \n"
        "BL    sched_load_psp              \n"
        "LDMIA R0!, {R4-R11, LR}           \n"
        "TST   LR, #0x10                   \n"
        "IT    EQ                          \n"
        "VLDMIAEQ R0!, {S16-S31}           \n"
        "MSR   PSP, R0                     \n"
        "BX    LR                          \n" /* exception return: the core pops the frame */
    );
}

/* ---- giving the CPU away ---- */

static void yield_now(void) {
    SCB_ICSR = ICSR_PENDSVSET;
    /* the exception is taken a few cycles later; without these barriers
     * the caller could run on for an instruction or two (FreeRTOS's
     * portYIELD has the same two lines) */
    __asm volatile("DSB \n ISB" ::: "memory");
}

/* Block the calling task for `ticks` ticks (1 ms each) and switch away
 * at once instead of waiting for the tick to do it. */
static void task_delay(uint32_t ticks) {
    /* the two stores must not be split by a tick — one uninterruptible step */
    __asm volatile("CPSID i" ::: "memory");
    tasks[current_task].wake_tick = tick_count + ticks;
    tasks[current_task].state = TASK_BLOCKED;
    __asm volatile("CPSIE i" ::: "memory");
    yield_now();
}

/* CPU time, not wall time: the slides' delay. Under time-slicing a task
 * only gets 1 ms in every 4, so 1 ms of this stretches to ~4 ms on the
 * clock — SCHED_SPIN_DELAYS's whole lesson. */
static void spin_ms(uint32_t ms) {
    for (volatile uint32_t turns = 0; turns < ms * SPIN_PER_MS; turns++) {}
}

static void wait_ms(uint32_t ms) {
    if (sched_mode == SCHED_BLOCKING_DELAYS) {
        task_delay(ms);
    } else {
        spin_ms(ms);
    }
}

/* ---- the tasks ---- */

static uint32_t saved_exc_return(uint32_t task) {
    return ((const struct SavedContext*)tasks[task].psp)->exc_return;
}

/* Only ONE task prints: newlib's printf is not reentrant, and two tasks
 * inside it at once (a tick can land anywhere) would corrupt each other.
 * One line per ON edge, carrying every number the lesson is about. */
static void report_from_task1(void) {
    static uint32_t last_on_tick;
    static uint32_t last_idle_loops;
    uint32_t now = tick_count;
    uint32_t idle_now = idle_loops;

    printf("[T1] tick=%lu  on->on=%lu ticks  idle loops since=%lu  my FPCA=%lu  "
           "saved EXC_RETURN T2=0x%08lx T3=0x%08lx T4=0x%08lx\r\n",
           (unsigned long)now, (unsigned long)(now - last_on_tick),
           (unsigned long)(idle_now - last_idle_loops),
           (unsigned long)((read_control() & CONTROL_FPCA) ? 1 : 0),
           (unsigned long)saved_exc_return(2), (unsigned long)saved_exc_return(3),
           (unsigned long)saved_exc_return(4));
    last_on_tick = now;
    last_idle_loops = idle_now;
}

/* One float multiply is enough to set FPCA in task 2: from then on the
 * core stacks it with the extended frame and its saved EXC_RETURN reads
 * 0xFFFFFFED in task 1's report, while tasks 3 and 4 stay 0xFFFFFFFD. */
static void touch_the_fpu(void) {
    volatile float scaled = (float)tick_count * 0.5F;
    (void)scaled;
}

/* Every user task is this function; the forged frame's r0 tells it which
 * one it is — the way an RTOS hands a task its argument. */
static void blink_task(uint32_t task) {
    for (;;) {
        task_pin_write(task, true);
        if (task == 1) {
            report_from_task1();
        } else if (task == 2) {
            touch_the_fpu();
        }
        wait_ms(half_period_ms[task]);
        task_pin_write(task, false);
        wait_ms(half_period_ms[task]);
    }
}

/* Runs only when every user task is blocked. WFI sleeps the core until
 * the next interrupt — the tick, at most 1 ms away — so idle_loops ≈ the
 * number of idle milliseconds. */
static void idle_task(uint32_t unused) {
    (void)unused;
    for (;;) {
        idle_loops++;
        __asm volatile("WFI");
    }
}

/* A task's function must never return; if one does, its stacked lr
 * brings it here instead of into garbage. */
static void task_exit_trap(void) {
    for (;;) {}
}

/* ---- setting up ---- */

/* Write the 17-word saved state of a task that has never run, at the top
 * of its stack, so the first switch into it is an ordinary exception
 * return. The T bit must be set in the forged xPSR: on exception return
 * the core takes T from there, not from pc bit 0 (tbit.c's BX rule does
 * not apply), so pc is stored with bit 0 clear. */
static void forge_initial_context(uint32_t task, void (*entry)(uint32_t), uint32_t argument) {
    uint32_t* top = &task_stacks[task][STACK_WORDS];
    struct SavedContext* ctx = (struct SavedContext*)top - 1;

    *ctx = (struct SavedContext){0};
    ctx->exc_return = EXC_RETURN_THREAD_PSP_BASIC;
    ctx->frame.r0 = argument;
    ctx->frame.lr = (uint32_t)task_exit_trap;
    ctx->frame.pc = (uint32_t)entry & ~1UL;
    ctx->frame.xpsr = XPSR_THUMB;

    tasks[task] = (struct Tcb){.psp = (uint32_t)ctx, .state = TASK_READY};
}

/* Thread mode moves onto task 1's stack. CONTROL = 2: SPSEL=1 (PSP),
 * nPRIV=0, FPCA=0 — task 1 starts with a clean FPU state, like a forged
 * context would; its own first printf sets FPCA, and the report shows
 * it. ISB so what follows runs on the new stack selection. */
__attribute__((naked)) static void switch_to_psp(uint32_t psp __attribute__((unused))) {
    __asm volatile("MSR PSP, R0        \n"
                   "MOV R1, #2         \n"
                   "MSR CONTROL, R1    \n"
                   "ISB                \n"
                   "BX  LR             \n");
}

/* --------------------------------------------------------------------- */

void playing_with_scheduler(enum SchedMode mode) {
    uart2_init();
    sched_mode = mode;

    printf("\r\nLesson: the task scheduler — 4 tasks, 1 ms tick, PendSV does the switch\r\n");
    printf("mode: %s\r\n", mode == SCHED_BLOCKING_DELAYS
                               ? "BLOCKING delays — task_delay() gives the CPU away"
                               : "SPIN delays — tasks burn CPU to wait (slides' first build)");
    printf("SysTick: %lu Hz MSI / %lu Hz tick → RELOAD %lu (course: 16 MHz HSI → 16000)\r\n",
           CPU_HZ, TICK_HZ, SYSTICK_RELOAD);
    printf(
        "stacks: %u x %u KB in .bss from 0x%08lx — idle, T1..T4; 17-word forged context each\r\n",
        (unsigned)TASK_COUNT, (unsigned)(STACK_WORDS * 4U / 1024U),
        (unsigned long)(uintptr_t)task_stacks);
    printf(
        "pins: T1 PA5 (LD2/D13) 1000 ms, T2 PA6 (D12) 500, T3 PA7 (D11) 250, T4 PA9 (D8) 125\r\n");

    /* PendSV and SysTick at the lowest priority: the switch runs only
     * after every other interrupt is done (nvic.c round S, now for real) */
    SCB_SHPR3 |= SHPR3_PENDSV_LOWEST | SHPR3_SYSTICK_LOWEST;

    systick_init(); /* counts from here on; switching stays disarmed */
    uint32_t before = tick_count;
    spin_ms(100);
    printf("calibration: spin_ms(100) alone on the CPU = %lu ticks\r\n",
           (unsigned long)(tick_count - before));

    task_pins_init();
    forge_initial_context(IDLE_TASK, idle_task, 0);
    for (uint32_t task = 1; task < TASK_COUNT; task++) {
        forge_initial_context(task, blink_task, task);
    }

    printf("expect per T1 line: on->on ≈ %s; idle loops %s; T2's EXC_RETURN 0xFFFFFFFD → "
           "0xFFFFFFED once it has touched the FPU\r\n",
           mode == SCHED_BLOCKING_DELAYS ? "2000 ticks (+ this line's printf time)"
                                         : "7840+ ticks (2 s of CPU shared four ways — the rest is "
                                           "what 1000 switches/s cost at 4 MHz)",
           mode == SCHED_BLOCKING_DELAYS ? "≈ 1990 (asleep between wakes)" : "0 (never runs)");
    printf("switching thread mode to PSP, arming the switch, calling task 1...\r\n\r\n");

    switch_to_psp(tasks[1].psp);
    sched_started = 1; /* from here every tick may switch — PSP is valid now */
    blink_task(1);     /* task 1's forged frame is never popped: it is simply called */
}
