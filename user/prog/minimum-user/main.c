#include <stdint.h>
#include "minemu/user_abi.h"

#define MSH_LINE_MAX 25

static char line_buffer[MSH_LINE_MAX + 1];
static uint32_t line_len = 0;
static uint32_t overflow_count = 0;

static void uart_putc(char character)
{
    ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
          (void *)&character, (uint32_t)1);
}

static void uart_puts(const char *message)
{
    const char *end = message;

    while (*end != '\0')
    {
        end++;
    }

    ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
          (void *)message, (uint32_t)(end - message));
}

static int uart_getc(char *out)
{
    return ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ,
                 (void *)out, (uint32_t)1);
}

static void msh_execute_command(char *line)
{
    char *command = line;

    while (*command == ' ')
    {
        command++;
    }

    if (*command == '\0')
    {
        return;
    }

    if (command[0] == 'e' && command[1] == 'c' &&
        command[2] == 'h' && command[3] == 'o' &&
        (command[4] == ' ' || command[4] == '\0'))
    {
        char *text = command + 4;

        while (*text == ' ')
        {
            text++;
        }

        while (*text != '\0')
        {
            if (*text == ' ')
            {
                while (*text == ' ')
                {
                    text++;
                }
                if (*text != '\0')
                {
                    uart_putc(' ');
                }
            }
            else
            {
                uart_putc(*text);
                text++;
            }
        }
        uart_putc('\n');
    }
    else
    {
        uart_puts("command not found: ");

        char *p = command;

        while (*p != ' ' && *p != '\0')
        {
            uart_putc(*p);
            p++;
        }

        uart_putc('\n');
    }
}

void minemu_user_main(void)
{
    uart_puts("msh> ");

    for (;;)
    {
        char c;

        int result = uart_getc(&c);

        if (result < 0)
        {
            uart_puts("UART READ ERROR\n");
            for (;;)
            {
                __asm__ volatile("nop");
            }
        }

        if (result == 0)
        {
            for (volatile uint32_t i = 0; i < 1000; i++)
            {
                __asm__ volatile("nop");
            }
            continue;
        }

        if (result == 0)
        {
            for (volatile uint32_t i = 0; i < 1000; i++)
            {
                __asm__ volatile("nop");
            }
            continue;
        }

        if (c == '\n')
        {
            if (overflow_count == 0)
            {
                line_buffer[line_len] = '\0';
                msh_execute_command(line_buffer);
            }

            line_len = 0;
            overflow_count = 0;
            uart_puts("msh> ");
            continue;
        }

        if (c == 0x08 || c == 0x7f)
        {
            if (overflow_count > 0)
            {
                overflow_count--;
            }
            else if (line_len > 0)
            {
                line_len--;
            }

            continue;
        }

        if (line_len < MSH_LINE_MAX)
        {
            line_buffer[line_len++] = c;
        }
        else
        {
            overflow_count++;
        }
    }
}