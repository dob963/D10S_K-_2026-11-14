#include "py32f071_uvk1_v3.h"


#include <stddef.h>

/*
 * Minimal register views for the PY32F071 blocks used by this board port.
 * Addresses and offsets are checked against the pinned PY32F071xB CMSIS
 * device header recorded in docs/uvk1-v3-board-map-evidence.md.
 */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
    volatile uint32_t BRR;
} d10s_gpio_regs_t;

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} d10s_spi_regs_t;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t ICSCR;
    volatile uint32_t CFGR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t ECSCR;
    volatile uint32_t RESERVED1;
    volatile uint32_t CIER;
    volatile uint32_t CIFR;
    volatile uint32_t CICR;
    volatile uint32_t IOPRSTR;
    volatile uint32_t AHBRSTR;
    volatile uint32_t APBRSTR1;
    volatile uint32_t APBRSTR2;
    volatile uint32_t IOPENR;
    volatile uint32_t AHBENR;
    volatile uint32_t APBENR1;
    volatile uint32_t APBENR2;
} d10s_rcc_regs_t;

#define D10S_RCC   ((d10s_rcc_regs_t *)(uintptr_t)0x40021000u)
#define D10S_SPI1  ((d10s_spi_regs_t *)(uintptr_t)0x40013000u)
#define D10S_GPIOA ((d10s_gpio_regs_t *)(uintptr_t)0x50000000u)
#define D10S_GPIOB ((d10s_gpio_regs_t *)(uintptr_t)0x50000400u)
#define D10S_GPIOF ((d10s_gpio_regs_t *)(uintptr_t)0x50001400u)

#define D10S_RCC_GPIOA_EN (1u << 0)
#define D10S_RCC_GPIOB_EN (1u << 1)
#define D10S_RCC_GPIOF_EN (1u << 5)
#define D10S_RCC_SPI1_EN  (1u << 12)

#define D10S_SPI_CR1_CPHA (1u << 0)
#define D10S_SPI_CR1_CPOL (1u << 1)
#define D10S_SPI_CR1_MSTR (1u << 2)
#define D10S_SPI_CR1_BR_DIV64 ((5u) << 3)
#define D10S_SPI_CR1_SPE  (1u << 6)
#define D10S_SPI_CR1_SSI  (1u << 8)
#define D10S_SPI_CR1_SSM  (1u << 9)
#define D10S_SPI_SR_RXNE  (1u << 0)
#define D10S_SPI_SR_TXE   (1u << 1)
#define D10S_SPI_SR_BSY   (1u << 7)

#define D10S_PIN_PA5 5u
#define D10S_PIN_PA6 6u
#define D10S_PIN_PA7 7u
#define D10S_PIN_PB2 2u
#define D10S_PIN_PB8 8u
#define D10S_PIN_PB9 9u
#define D10S_PIN_PF8 8u
#define D10S_PIN_PF9 9u

#define D10S_SPI_TIMEOUT 100000u

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} d10s_systick_regs_t;

#define D10S_SYSTICK ((d10s_systick_regs_t *)(uintptr_t)0xE000E010u)
#define D10S_SYSTICK_RELOAD 0x00FFFFFFu
#define D10S_SYSTICK_ENABLE (1u << 0)
#define D10S_SYSTICK_CORE_CLOCK (1u << 2)
#define D10S_MAX_DELAY_CHUNK 65535u
#define D10S_SYSTICK_POLL_BUDGET 1000000u

static bool systick_delay_ticks(uint16_t ticks, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    uint32_t start;
    uint32_t current;
    uint32_t budget = D10S_SYSTICK_POLL_BUDGET;
    if (board == NULL || !board->systick_ready) return false;
    if (ticks == 0u) return true;

    start = D10S_SYSTICK->VAL & D10S_SYSTICK_RELOAD;
    while (budget-- != 0u) {
        current = D10S_SYSTICK->VAL & D10S_SYSTICK_RELOAD;
        if (((start - current) & D10S_SYSTICK_RELOAD) >= ticks)
            return true;
    }
    return false;
}

static bool systick_delay_ms(uint16_t milliseconds, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    uint64_t remaining;
    if (board == NULL || !board->systick_ready) return false;
    remaining = ((uint64_t)board->core_clock_hz * milliseconds + 999u) /
                1000u;
    while (remaining != 0u) {
        const uint16_t chunk = (uint16_t)(
            remaining > D10S_MAX_DELAY_CHUNK
                ? D10S_MAX_DELAY_CHUNK : remaining);
        if (!systick_delay_ticks(chunk, board)) return false;
        remaining -= chunk;
    }
    return true;
}

bool d10s_py32f071_uvk1_v3_systick_init(
    d10s_py32f071_uvk1_v3_t *board,
    uint32_t measured_core_clock_hz)
{
    if (board == NULL || measured_core_clock_hz < 1000000u ||
        measured_core_clock_hz > 72000000u) return false;

    D10S_SYSTICK->CTRL = 0u;
    D10S_SYSTICK->LOAD = D10S_SYSTICK_RELOAD;
    D10S_SYSTICK->VAL = 0u;
    D10S_SYSTICK->CTRL = D10S_SYSTICK_ENABLE | D10S_SYSTICK_CORE_CLOCK;

    board->core_clock_hz = measured_core_clock_hz;
    board->systick_last = D10S_SYSTICK->VAL & D10S_SYSTICK_RELOAD;
    board->milliseconds = 0u;
    board->cycle_remainder = 0u;
    board->systick_ready = true;
    return true;
}

uint32_t d10s_py32f071_uvk1_v3_millis(d10s_py32f071_uvk1_v3_t *board)
{
    uint32_t current;
    uint32_t elapsed;
    uint64_t scaled_cycles;
    if (board == NULL || !board->systick_ready) return 0u;

    current = D10S_SYSTICK->VAL & D10S_SYSTICK_RELOAD;
    elapsed = (board->systick_last - current) & D10S_SYSTICK_RELOAD;
    board->systick_last = current;
    scaled_cycles = board->cycle_remainder + (uint64_t)elapsed * 1000u;
    board->milliseconds += (uint32_t)(scaled_cycles /
                                      board->core_clock_hz);
    board->cycle_remainder = scaled_cycles % board->core_clock_hz;
    return board->milliseconds;
}


static void pin_write(d10s_gpio_regs_t *gpio, uint8_t pin, bool high);
static bool gpio_init(d10s_py32f071_uvk1_v3_t *board);

static bool systick_delay_us(d10s_py32f071_uvk1_v3_t *board,
                             uint16_t microseconds)
{
    uint64_t remaining;
    if (board == NULL || !board->systick_ready) return false;
    remaining = ((uint64_t)board->core_clock_hz * microseconds + 999999u) /
                1000000u;
    while (remaining != 0u) {
        const uint16_t chunk = (uint16_t)(
            remaining > D10S_MAX_DELAY_CHUNK
                ? D10S_MAX_DELAY_CHUNK : remaining);
        if (!systick_delay_ticks(chunk, board)) return false;
        remaining -= chunk;
    }
    return true;
}

bool d10s_py32f071_uvk1_v3_keypad_init(
    d10s_py32f071_uvk1_v3_t *board)
{
    if (board == NULL || !gpio_init(board) || !board->systick_ready)
        return false;
    board->keypad_ready = true;
    return true;
}

bool d10s_py32f071_uvk1_v3_keypad_poll(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_spectrum_key_t *key)
{
    static const d10s_py32f071_uvk1_v3_key_t keymap[5][4] = {
        {D10S_KEY_SIDE1, D10S_KEY_SIDE2, D10S_KEY_NONE, D10S_KEY_NONE},
        {D10S_KEY_MENU, D10S_KEY_1, D10S_KEY_4, D10S_KEY_7},
        {D10S_KEY_UP, D10S_KEY_2, D10S_KEY_5, D10S_KEY_8},
        {D10S_KEY_DOWN, D10S_KEY_3, D10S_KEY_6, D10S_KEY_9},
        {D10S_KEY_EXIT, D10S_KEY_STAR, D10S_KEY_0, D10S_KEY_F}
    };
    const uint32_t row_mask = 0xF000u;
    d10s_py32f071_uvk1_v3_key_t detected = D10S_KEY_NONE;
    bool multiple = false;
    uint8_t column;

    if (board == NULL || key == NULL || !board->keypad_ready ||
        !board->systick_ready) return false;

    for (column = 0u; column < 5u; ++column) {
        uint32_t stable_rows = 0u;
        bool stable = true;
        uint8_t sample;
        uint8_t pressed_row = 0u;
        uint8_t pressed_count = 0u;

        D10S_GPIOB->BSRR = (1u << 6) | (1u << 5) | (1u << 4) | (1u << 3);
        if (column != 0u) {
            const uint8_t pin = (uint8_t)(7u - column);
            pin_write(D10S_GPIOB, pin, false);
        }

        for (sample = 0u; sample < 3u; ++sample) {
            uint32_t rows;
            if (!systick_delay_us(board, 10u)) {
                D10S_GPIOB->BSRR = (1u << 6) | (1u << 5) |
                                   (1u << 4) | (1u << 3);
                return false;
            }
            rows = D10S_GPIOB->IDR & row_mask;
            if (sample == 0u) stable_rows = rows;
            else if (rows != stable_rows) stable = false;
        }

        if (stable) {
            const uint32_t pressed = (~stable_rows) & row_mask;
            uint8_t row;
            for (row = 0u; row < 4u; ++row) {
                const uint32_t mask = 1u << (15u - row);
                if ((pressed & mask) != 0u) {
                    pressed_row = row;
                    ++pressed_count;
                }
            }
            if (pressed_count == 1u) {
                const d10s_py32f071_uvk1_v3_key_t candidate =
                    keymap[column][pressed_row];
                if (candidate != D10S_KEY_NONE) {
                    if (detected != D10S_KEY_NONE) multiple = true;
                    detected = candidate;
                }
            } else if (pressed_count > 1u) {
                multiple = true;
            }
        }
    }

    D10S_GPIOB->BSRR = (1u << 6) | (1u << 5) | (1u << 4) | (1u << 3);
    *key = multiple ? D10S_KEY_NONE : detected;
    return true;
}

bool d10s_py32f071_uvk1_v3_build_bk4829_timing(
    const d10s_py32f071_uvk1_v3_t *board,
    d10s_bk4829_bus_timing_t *timing)
{
    if (board == NULL || !board->systick_ready || timing == NULL)
        return false;
    return d10s_bk4829_build_timing(board->core_clock_hz, timing);
}

bool d10s_py32f071_uvk1_v3_display_port_init_systick(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_st7565_port_t *port)
{
    if (board == NULL || !board->systick_ready) return false;
    return d10s_py32f071_uvk1_v3_display_port_init(
        board, port, systick_delay_ms, board);
}

bool d10s_py32f071_uvk1_v3_radio_bus_port_init_systick(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_bk4829_bus_port_t *port)
{
    if (board == NULL || !board->systick_ready) return false;
    return d10s_py32f071_uvk1_v3_radio_bus_port_init(
        board, port, systick_delay_ticks, board);
}

static void pin_write(d10s_gpio_regs_t *gpio, uint8_t pin, bool high)
{
    gpio->BSRR = high ? (1u << pin) : (1u << (pin + 16u));
}

static void pin_mode(d10s_gpio_regs_t *gpio, uint8_t pin, uint32_t mode)
{
    const uint32_t shift = (uint32_t)pin * 2u;
    gpio->MODER = (gpio->MODER & ~(3u << shift)) | ((mode & 3u) << shift);
}

static void pin_speed_high(d10s_gpio_regs_t *gpio, uint8_t pin)
{
    const uint32_t shift = (uint32_t)pin * 2u;
    gpio->OSPEEDR = (gpio->OSPEEDR & ~(3u << shift)) | (3u << shift);
}

static void pin_no_pull(d10s_gpio_regs_t *gpio, uint8_t pin)
{
    const uint32_t shift = (uint32_t)pin * 2u;
    gpio->PUPDR &= ~(3u << shift);
}

static void pin_pull_up(d10s_gpio_regs_t *gpio, uint8_t pin)
{
    const uint32_t shift = (uint32_t)pin * 2u;
    gpio->PUPDR = (gpio->PUPDR & ~(3u << shift)) | (1u << shift);
}

static void pin_output_init(d10s_gpio_regs_t *gpio, uint8_t pin, bool initial_high)
{
    pin_write(gpio, pin, initial_high);
    gpio->OTYPER &= ~(1u << pin);
    pin_speed_high(gpio, pin);
    pin_no_pull(gpio, pin);
    pin_mode(gpio, pin, 1u);
}

static void pin_alternate_init(d10s_gpio_regs_t *gpio, uint8_t pin,
                               bool pull_up)
{
    const uint32_t bank = (uint32_t)pin >> 3;
    const uint32_t shift = ((uint32_t)pin & 7u) * 4u;
    gpio->AFR[bank] &= ~(15u << shift); /* AF0: SPI1 on PA5/PA7 */
    gpio->OTYPER &= ~(1u << pin);
    pin_speed_high(gpio, pin);
    if (pull_up) pin_pull_up(gpio, pin);
    else pin_no_pull(gpio, pin);
    pin_mode(gpio, pin, 2u);
}

static bool gpio_init(d10s_py32f071_uvk1_v3_t *board)
{
    if (board == NULL) return false;
    D10S_RCC->IOPENR |= D10S_RCC_GPIOA_EN |
                       D10S_RCC_GPIOB_EN |
                       D10S_RCC_GPIOF_EN;

    /* Safe idle levels are established before the pins become outputs. */
    pin_output_init(D10S_GPIOA, D10S_PIN_PA6, false); /* LCD A0: command */
    pin_output_init(D10S_GPIOB, D10S_PIN_PB2, true);  /* LCD CS: inactive */
    pin_output_init(D10S_GPIOB, D10S_PIN_PB8, true);  /* BK SCL: idle high */
    pin_output_init(D10S_GPIOB, D10S_PIN_PB9, true);  /* BK SDA: idle high */
    pin_output_init(D10S_GPIOF, D10S_PIN_PF8, false); /* Backlight held off */
    pin_output_init(D10S_GPIOF, D10S_PIN_PF9, true);  /* BK CSN: inactive */

    /* Candidate K1 keypad matrix: PB6..PB3 columns, PB15..PB12 rows. */
    pin_output_init(D10S_GPIOB, 6u, true);
    pin_output_init(D10S_GPIOB, 5u, true);
    pin_output_init(D10S_GPIOB, 4u, true);
    pin_output_init(D10S_GPIOB, 3u, true);
    pin_pull_up(D10S_GPIOB, 15u);
    pin_pull_up(D10S_GPIOB, 14u);
    pin_pull_up(D10S_GPIOB, 13u);
    pin_pull_up(D10S_GPIOB, 12u);
    pin_mode(D10S_GPIOB, 15u, 0u);
    pin_mode(D10S_GPIOB, 14u, 0u);
    pin_mode(D10S_GPIOB, 13u, 0u);
    pin_mode(D10S_GPIOB, 12u, 0u);

    board->gpio_ready = true;
    return true;
}

static bool spi1_init(void)
{
    if ((D10S_RCC->IOPENR & D10S_RCC_GPIOA_EN) == 0u) return false;
    D10S_RCC->APBENR2 |= D10S_RCC_SPI1_EN;

    pin_alternate_init(D10S_GPIOA, D10S_PIN_PA5, true);  /* SPI1 SCK */
    pin_alternate_init(D10S_GPIOA, D10S_PIN_PA7, false); /* SPI1 MOSI */

    D10S_SPI1->CR1 = 0u;
    D10S_SPI1->CR2 = 0u;
    D10S_SPI1->CR1 = D10S_SPI_CR1_CPHA |
                     D10S_SPI_CR1_CPOL |
                     D10S_SPI_CR1_MSTR |
                     D10S_SPI_CR1_BR_DIV64 |
                     D10S_SPI_CR1_SSI |
                     D10S_SPI_CR1_SSM;
    D10S_SPI1->CR1 |= D10S_SPI_CR1_SPE;
    return true;
}

static bool display_chip_select(bool asserted, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    if (board == NULL || !board->gpio_ready) return false;
    pin_write(D10S_GPIOB, D10S_PIN_PB2, !asserted);
    return true;
}

static bool display_data_mode(bool data, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    if (board == NULL || !board->gpio_ready) return false;
    pin_write(D10S_GPIOA, D10S_PIN_PA6, data);
    return true;
}

static bool display_write_byte(uint8_t value, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    uint32_t timeout;
    volatile uint32_t received = 0u;
    if (board == NULL || !board->spi_ready) return false;

    if ((D10S_SPI1->SR & D10S_SPI_SR_RXNE) != 0u)
        received = D10S_SPI1->DR;
    (void)received;

    for (timeout = 0u; timeout < D10S_SPI_TIMEOUT; ++timeout)
        if ((D10S_SPI1->SR & D10S_SPI_SR_TXE) != 0u) break;
    if (timeout == D10S_SPI_TIMEOUT) return false;

    D10S_SPI1->DR = value;
    for (timeout = 0u; timeout < D10S_SPI_TIMEOUT; ++timeout)
        if ((D10S_SPI1->SR & D10S_SPI_SR_RXNE) != 0u) break;
    if (timeout == D10S_SPI_TIMEOUT) return false;

    received = D10S_SPI1->DR;
    (void)received;
    for (timeout = 0u; timeout < D10S_SPI_TIMEOUT; ++timeout)
        if ((D10S_SPI1->SR & D10S_SPI_SR_BSY) == 0u) return true;
    return false;
}

static bool display_delay_ms(uint16_t milliseconds, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    return board != NULL && board->delay_ms != NULL &&
           board->delay_ms(milliseconds, board->delay_ms_context);
}

static bool radio_set_csn(bool high, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    if (board == NULL || !board->gpio_ready) return false;
    pin_write(D10S_GPIOF, D10S_PIN_PF9, high);
    return true;
}

static bool radio_set_clock(bool high, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    if (board == NULL || !board->gpio_ready) return false;
    pin_write(D10S_GPIOB, D10S_PIN_PB8, high);
    return true;
}

static bool radio_set_data_output(bool output, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    if (board == NULL || !board->gpio_ready) return false;
    if (output) {
        D10S_GPIOB->OTYPER &= ~(1u << D10S_PIN_PB9);
        pin_speed_high(D10S_GPIOB, D10S_PIN_PB9);
        pin_no_pull(D10S_GPIOB, D10S_PIN_PB9);
        pin_mode(D10S_GPIOB, D10S_PIN_PB9, 1u);
    } else {
        pin_mode(D10S_GPIOB, D10S_PIN_PB9, 0u);
    }
    return true;
}

static bool radio_write_data(bool high, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    if (board == NULL || !board->gpio_ready) return false;
    pin_write(D10S_GPIOB, D10S_PIN_PB9, high);
    return true;
}

static bool radio_read_data(bool *high, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    if (board == NULL || !board->gpio_ready || high == NULL) return false;
    *high = (D10S_GPIOB->IDR & (1u << D10S_PIN_PB9)) != 0u;
    return true;
}

static bool radio_delay_ticks(uint16_t ticks, void *context)
{
    d10s_py32f071_uvk1_v3_t *board =
        (d10s_py32f071_uvk1_v3_t *)context;
    return board != NULL && board->delay_ticks != NULL &&
           board->delay_ticks(ticks, board->delay_ticks_context);
}

bool d10s_py32f071_uvk1_v3_display_port_init(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_st7565_port_t *port,
    d10s_py32_delay_ms_fn delay_ms,
    void *delay_context)
{
    if (board == NULL || port == NULL || delay_ms == NULL ||
        !gpio_init(board)) return false;
    board->delay_ms = delay_ms;
    board->delay_ms_context = delay_context;
    if (!spi1_init()) return false;
    board->spi_ready = true;

    port->chip_select = display_chip_select;
    port->data_mode = display_data_mode;
    port->write_byte = display_write_byte;
    port->delay_ms = display_delay_ms;
    port->context = board;
    return true;
}

bool d10s_py32f071_uvk1_v3_radio_bus_port_init(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_bk4829_bus_port_t *port,
    d10s_py32_delay_ticks_fn delay_ticks,
    void *delay_context)
{
    if (board == NULL || port == NULL || delay_ticks == NULL ||
        !gpio_init(board)) return false;
    board->delay_ticks = delay_ticks;
    board->delay_ticks_context = delay_context;

    port->set_csn_high = radio_set_csn;
    port->set_clock_high = radio_set_clock;
    port->set_data_output = radio_set_data_output;
    port->write_data_high = radio_write_data;
    port->read_data_high = radio_read_data;
    port->delay_ticks = radio_delay_ticks;
    port->context = board;
    return true;
}
