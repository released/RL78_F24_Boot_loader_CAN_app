#ifndef APP_RUNTIME_LOG_H
#define APP_RUNTIME_LOG_H

#include <limits.h>
#if ULONG_MAX != 0xFFFFFFFFUL
#error App_runtime_counter_requires_a_32_bit_unsigned_long
#endif

/* User-editable observation tag, independent of APP_IMAGE_VERSION.
 * Select "V1" or "V2", then clean rebuild this App only.
 * The one-second counter starts at zero after each App startup.
 * Formatting and UART output run in main, never in the timer ISR.
 */
#define APP_RUNTIME_LOG_TAG "V1"
/* Example alternative: #define APP_RUNTIME_LOG_TAG "V2" */

/* Ten decimal digits for a 32-bit unsigned long, plus the terminator. */
#define APP_RUNTIME_COUNTER_BUFFER_SIZE (11U)
void App_FormatRuntimeCounter(char *output, unsigned long counter);

#endif
