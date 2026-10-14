#ifndef SYSCALL_NUMBERS_H
#define SYSCALL_NUMBERS_H

#define SYS_IOCTL 1

#define IOCTL_RNG 0x10003000
#define IOCTL_UART0 0x10004000
#define IOCTL_UART1 0x10005000
#define IOCTL_TRACE 0x1000f000

#define RNG_SET_SEED 0
#define RNG_GET_DATA 1
#define UART_READ 0
#define UART_WRITE 1
#define TRACE_EVENT 0

int ioctl(int fd, int op, ...);

#endif