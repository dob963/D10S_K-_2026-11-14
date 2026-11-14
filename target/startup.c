#include <stdint.h>

typedef void (*d10s_isr_handler_t)(void);

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

int main(void);

void Reset_Handler(void);
void Default_Handler(void);

__attribute__((noreturn))
void Default_Handler(void)
{
    for (;;) {
        __asm volatile ("nop");
    }
}

typedef struct {
    const uint32_t *initial_sp;
    d10s_isr_handler_t reset;
    d10s_isr_handler_t nmi;
    d10s_isr_handler_t hardfault;
    d10s_isr_handler_t reserved_4_10[7];
    d10s_isr_handler_t svc;
    d10s_isr_handler_t reserved_12_13[2];
    d10s_isr_handler_t pendsv;
    d10s_isr_handler_t systick;
    d10s_isr_handler_t external_irq[32];
} d10s_vector_table_t;

__attribute__((section(".isr_vector"), used, aligned(128)))
const d10s_vector_table_t d10s_vectors = {
    .initial_sp = &_estack,
    .reset = Reset_Handler,
    .nmi = Default_Handler,
    .hardfault = Default_Handler,
    .reserved_4_10 = {0},
    .svc = Default_Handler,
    .reserved_12_13 = {0},
    .pendsv = Default_Handler,
    .systick = Default_Handler,
    .external_irq = {
        [0] = Default_Handler,  [1] = Default_Handler,
        [2] = Default_Handler,  [3] = Default_Handler,
        [4] = Default_Handler,  [5] = Default_Handler,
        [6] = Default_Handler,  [7] = Default_Handler,
        [8] = Default_Handler,  [9] = Default_Handler,
        [10] = Default_Handler, [11] = Default_Handler,
        [12] = Default_Handler, [13] = Default_Handler,
        [14] = Default_Handler, [15] = Default_Handler,
        [16] = Default_Handler, [17] = Default_Handler,
        [18] = Default_Handler, [19] = Default_Handler,
        [20] = Default_Handler, [21] = Default_Handler,
        [22] = Default_Handler, [23] = Default_Handler,
        [24] = Default_Handler, [25] = Default_Handler,
        [26] = Default_Handler, [27] = Default_Handler,
        [28] = Default_Handler, [29] = Default_Handler,
        [30] = 0, /* Reserved: PY32F071 has no IRQ number 30. */
        [31] = Default_Handler /* USB_IRQn */
    }
};

__attribute__((noreturn))
void Reset_Handler(void)
{
    uint32_t *source = &_sidata;
    uint32_t *destination;

    /* Relocate VTOR to this application image (bootloader offset 0x2800). */
    *(volatile uint32_t *)(uintptr_t)0xE000ED08u = 0x08002800u;

    for (destination = &_sdata; destination < &_edata; ++destination)
        *destination = *source++;

    for (destination = &_sbss; destination < &_ebss; ++destination)
        *destination = 0u;

    (void)main();
    for (;;) {
        __asm volatile ("nop");
    }
}
