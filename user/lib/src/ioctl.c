#include <stdarg.h>
#include <stdint.h>
#include "minemu/syscall_numbers.h"

int ioctl(int fd, int op, ...)
{
    uintptr_t arg2 = 0;
    uintptr_t arg3 = 0;

    if (fd == IOCTL_RNG)
    {
        if (op != RNG_SET_SEED && op != RNG_GET_DATA)
        {
            return -1;
        }
    }
    else if (fd == IOCTL_UART0 || fd == IOCTL_UART1)
    {
        if (op != UART_READ && op != UART_WRITE)
        {
            return -1;
        }
    }
    else if (fd == IOCTL_TRACE)
    {
        if (op != TRACE_EVENT)
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    va_list args;
    va_start(args, op);

    if (fd == IOCTL_RNG)
    {
        if (op == RNG_SET_SEED)
        {
            arg2 = va_arg(args, unsigned int);
        }
        else
        {
            arg2 = (uintptr_t)va_arg(args, unsigned int *);
        }
    }
    else if (fd == IOCTL_UART0 || fd == IOCTL_UART1)
    {
        arg2 = (uintptr_t)va_arg(args, char *);
        arg3 = (uintptr_t)va_arg(args, int);
    }
    else if (fd == IOCTL_TRACE)
    {
        arg2 = va_arg(args, unsigned int);
    }

    va_end(args);

    register int r0 __asm__("r0") = fd;
    register int r1 __asm__("r1") = op;
    register uintptr_t r2 __asm__("r2") = arg2;
    register uintptr_t r3 __asm__("r3") = arg3;
    register int r7 __asm__("r7") = SYS_IOCTL;

    __asm__ volatile(
        "svc #0"
        : "+r"(r0)
        : "r"(r1), "r"(r2), "r"(r3), "r"(r7)
        : "memory", "cc");

    return r0;
}