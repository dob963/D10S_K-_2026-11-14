#include <stddef.h>
#include <stdint.h>

/*
 * Small freestanding memory primitives for the target image.
 * Volatile byte accesses prevent the optimizer from folding these routines
 * back into calls to the same library symbols.
 */
void *memcpy(void *destination, const void *source, size_t count)
{
    volatile uint8_t *out = (volatile uint8_t *)destination;
    const volatile uint8_t *in = (const volatile uint8_t *)source;
    size_t i;
    for (i = 0u; i < count; ++i) out[i] = in[i];
    return destination;
}

void *memset(void *destination, int value, size_t count)
{
    volatile uint8_t *out = (volatile uint8_t *)destination;
    size_t i;
    for (i = 0u; i < count; ++i) out[i] = (uint8_t)value;
    return destination;
}

void *__aeabi_memcpy(void *destination, const void *source, size_t count)
{
    return memcpy(destination, source, count);
}

void *__aeabi_memcpy4(void *destination, const void *source, size_t count)
{
    return memcpy(destination, source, count);
}

void *__aeabi_memcpy8(void *destination, const void *source, size_t count)
{
    return memcpy(destination, source, count);
}

void __aeabi_memset(void *destination, size_t count, int value)
{
    (void)memset(destination, value, count);
}

void __aeabi_memset4(void *destination, size_t count, int value)
{
    (void)memset(destination, value, count);
}

void __aeabi_memset8(void *destination, size_t count, int value)
{
    (void)memset(destination, value, count);
}

void __aeabi_memclr(void *destination, size_t count)
{
    (void)memset(destination, 0, count);
}

void __aeabi_memclr4(void *destination, size_t count)
{
    (void)memset(destination, 0, count);
}

void __aeabi_memclr8(void *destination, size_t count)
{
    (void)memset(destination, 0, count);
}
