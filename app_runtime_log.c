#include "app_runtime_log.h"

/* The caller provides APP_RUNTIME_COUNTER_BUFFER_SIZE writable bytes. */
void App_FormatRuntimeCounter(char *output, unsigned long counter)
{
    char reverse[10];
    unsigned char count, index;

    count = 0U;
    do {
        reverse[count++] = (char)('0' + (counter % 10UL));
        counter /= 10UL;
    } while (counter != 0UL);

    index = 0U;
    while (count != 0U) {
        output[index++] = reverse[--count];
    }
    output[index] = '\0';
}
