#include "minemu/boot.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/platform.h"
#include "minemu/irq.h"

#define UART_BUFFER_SIZE 64

static volatile char uart_buffer[UART_BUFFER_SIZE];
static volatile uint32_t uart_head = 0;
static volatile uint32_t uart_tail = 0;

static void uart_putc(char character) {
    while ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) == 0) {
    }

    MINEMU_UART0->tx_data = (uint32_t)(uint8_t)character;
}

static void uart_puts(const char *message) {
    while (*message != '\0') {
        uart_putc(*message);
        ++message;
    }
}

static void uart_irq_handler(void) {
    while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) {
        char c = (char)(MINEMU_UART0->rx_data & 0xff);

        uint32_t next = (uart_head + 1) % UART_BUFFER_SIZE;

        if (next != uart_tail) {
            uart_buffer[uart_head] = c;
            uart_head = next;
        }

        uart_putc('.');
    }
}

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame) {

    uint32_t source = (uint32_t)frame->exception_id;

    if (source == MINEMU_IRQ_UART0) {
        uart_irq_handler();
    }

    MINEMU_INTERRUPT->eoi = source;

    return frame;
}

void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000)) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }
    uart_puts("hello world\n");

    MINEMU_INTERRUPT->enable =UINT32_C(1) << MINEMU_IRQ_UART0;

    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE;

    minemu_irq_enable();

    for (;;) {
        __asm__ volatile("nop");
    }

    // minemu_fail_stop();
}
