/* Lesson: the preprocessor — a text machine that runs BEFORE the compiler.
 *
 * Every line starting with # is handled in a separate first pass that
 * only cuts and pastes text. The compiler never sees your macros — it
 * sees the text they left behind. That explains everything here:
 *
 *   #define NAME value   -> every later NAME becomes that text. Not a
 *                           variable: no RAM, no type, no address.
 *   #define F(x) ...     -> function-like macro: paste with arguments.
 *                           Powerful, and full of traps (section 2).
 *   #                    -> inside a macro: turn an argument into a
 *                           string literal ("stringize", section 1).
 *   ##                   -> glue two tokens into one name (section 3).
 *   #if / #ifdef / #else / #endif
 *                        -> conditional COMPILATION: code in the losing
 *                           branch is not slow, not skipped — it does
 *                           not exist in the program at all.
 *   defined(X)           -> inside #if: true if X is a macro. Combines:
 *                           #if defined(DEBUG) && !defined(NDEBUG)
 *
 * You have used all of this all course without ceremony: the include
 * guards in every header under Inc/ are #ifndef; CMSIS register masks are
 * #defines; and the build passes -DSTM32L476xx, which the ST headers
 * test with #if defined(...) ladders to pick our exact chip — and
 * -DDEBUG in Debug builds, which this lesson exploits below. */

#include "preproc.h"
#include "stm32l476xx.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>

#define CPU_HZ 4000000u

/* THE TRAP: without parentheses the argument text merges with the
 * surrounding expression under normal precedence rules. */
#define BAD_SQUARE(x) (x * x)
#define SQUARE(x) ((x) * (x))
/* Second trap, told not shown (running it is undefined behavior):
 * SQUARE(i++) pastes to ((i++) * (i++)) — i incremented twice per
 * "call". Macros re-evaluate their arguments; functions never do. */

/* # in action: SHOW prints an expression's own source text next to its
 * value. #expr becomes the string BEFORE the macro is expanded. */
#define SHOW(expr) printf("   " #expr " = %lu\r\n", (unsigned long)(expr))

/* ## in action: build a CMSIS mask name from a pin number.
 * ODR_BIT(5) becomes the token GPIO_ODR_OD5. */
#define ODR_BIT(n) GPIO_ODR_OD##n

/* A real embedded pattern: a log macro that exists only in Debug builds.
 * flags.cmake passes -DDEBUG for the Debug config, so here LOG prints
 * (with a file:line prefix for free); in a Release build the whole call
 * would leave zero bytes in flash. The do { } while (0) wrapper makes
 * the macro behave like one statement even inside an unbraced if/else. */
#ifdef DEBUG
#define LOG(...)                                                     \
  do {                                                               \
    printf("   [%s:%d] ", __FILE_NAME__, __LINE__);                  \
    printf(__VA_ARGS__);                                             \
    printf("\r\n");                                                  \
  } while (0)
#else
#define LOG(...)                                                     \
  do {                                                               \
  } while (0)
#endif

/* VERBOSE is not defined, so VLOG compiles to nothing. Exercise: add
 * `#define VERBOSE` above this block and rebuild — the hidden line
 * appears. That is a feature switch with zero runtime cost. */
#ifdef VERBOSE
#define VLOG(...) LOG(__VA_ARGS__)
#else
#define VLOG(...)                                                    \
  do {                                                               \
  } while (0)
#endif

/* Numeric conditions work too — graded feature levels, not just on/off. */
#define FEATURE_LEVEL 2

void playing_with_preproc(void)
{
  uart2_init();

  printf("\r\n== preprocessor: the compile-time text machine ==\r\n");

  printf("1) macros are text, not variables (SHOW uses # to say so)\r\n");
  SHOW(CPU_HZ);
  SHOW(CPU_HZ / 1000u);

  printf("2) function-like macro traps\r\n");
  SHOW(BAD_SQUARE(2 + 3)); /* pastes to (2 + 3 * 2 + 3) = 11 */
  SHOW(SQUARE(2 + 3));     /* ((2 + 3) * (2 + 3)) = 25 */

  printf("3) ## glues names\r\n");
  SHOW(ODR_BIT(5)); /* the token GPIO_ODR_OD5, value 1 << 5 */

  printf("4) build fingerprint (predefined macros)\r\n");
  printf("   compiled %s %s — %s line %d — GCC %d.%d\r\n", __DATE__,
         __TIME__, __FILE_NAME__, __LINE__, __GNUC__, __GNUC_MINOR__);

  printf("5) conditional compilation\r\n");
#if defined(DEBUG) && !defined(NDEBUG)
  printf("   Debug build (-DDEBUG from flags.cmake)\r\n");
#else
  printf("   Release build\r\n");
#endif

  LOG("LOG is alive — and knows where it was written");
  VLOG("invisible until you #define VERBOSE and rebuild");

#if FEATURE_LEVEL >= 2
  printf("   FEATURE_LEVEL %d >= 2: this line was compiled in\r\n",
         FEATURE_LEVEL);
#endif

#if 0
  printf("never compiled — #if 0 is the classic way to park a block\r\n");
#endif

  printf("   and -DSTM32L476xx picked our chip out of the CMSIS\r\n"
         "   #if ladder before a single line of it was compiled\r\n");

  printf("done — course complete\r\n");
}
