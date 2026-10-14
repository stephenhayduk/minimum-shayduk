#include <stdint.h>
#include "minemu/syscall_numbers.h"

void minemu_user_main(void)
{
    char message[] = "Hello world!\n";
    ioctl(IOCTL_UART0, UART_WRITE,
          message, (int)(sizeof(message) - 1));
    for (;;)
    {
        __asm__ volatile("nop");
    }
}
