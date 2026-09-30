#include "minemu/boot.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/platform.h"
#include "minemu/irq.h"

#define UART_BUFFER_SIZE 64
#define MSH_LINE_MAX 20

static volatile char uart_buffer[UART_BUFFER_SIZE];
static volatile uint32_t uart_head = 0;
static volatile uint32_t uart_tail = 0;
char line_buffer[MSH_LINE_MAX + 1];
uint32_t line_len = 0;
uint32_t overflow_count = 0;

static void uart_putc(char character)
{
    while ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) == 0)
    {
    }

    MINEMU_UART0->tx_data = (uint32_t)(uint8_t)character;
}

static void uart_puts(const char *message)
{
    while (*message != '\0')
    {
        uart_putc(*message);
        ++message;
    }
}

static int uart_getc(char *out)
{
    minemu_irq_disable();

    if (uart_head == uart_tail)
    {
        minemu_irq_enable();
        return 0;
    }

    *out = uart_buffer[uart_tail];
    uart_tail = (uart_tail + 1) % UART_BUFFER_SIZE;

    minemu_irq_enable();
    return 1;
}

static void uart_irq_handler(void)
{
    while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY)
    {
        char c = (char)(MINEMU_UART0->rx_data & 0xff);

        uint32_t next = (uart_head + 1) % UART_BUFFER_SIZE;

        if (next != uart_tail)
        {
            uart_buffer[uart_head] = c;
            uart_head = next;
        }
    }
}

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame)
{

    uint32_t source = (uint32_t)frame->exception_id;

    if (source == MINEMU_IRQ_UART0)
    {
        uart_irq_handler();
    }

    MINEMU_INTERRUPT->eoi = source;

    return frame;
}

void msh_execute_command(char *line)
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

    if (command[0] == 'e' &&
        command[1] == 'c' &&
        command[2] == 'h' &&
        command[3] == 'o' &&
        (command[4] == ' ' || command[4] == '\0'))
    {
        char *text = command + 4;

        while (*text == ' ')
        {
            text++;
        }

        uart_puts(text);
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

void minemu_kernel_main(const struct minemu_boot_info *boot_info)
{
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000))
    {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }
    uart_puts("msh> ");

    MINEMU_INTERRUPT->enable = UINT32_C(1) << MINEMU_IRQ_UART0;

    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE;

    minemu_irq_enable();

    for (;;)
    {
        char c;

        if (!uart_getc(&c))
        {
            continue;
        }

        if (c == '\n')
        {
            if (overflow_count > 0)
            {
                uart_puts("Error: input cannot exceed 20 bytes\n");
            }
            else
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

    // minemu_fail_stop();
}
