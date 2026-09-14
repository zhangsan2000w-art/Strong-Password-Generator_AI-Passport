#include <stdint.h>

static uint32_t s_host_rng = 0x243f6a88u;

uint32_t passport_random_u32(void)
{
    s_host_rng ^= s_host_rng << 13;
    s_host_rng ^= s_host_rng >> 17;
    s_host_rng ^= s_host_rng << 5;
    return s_host_rng;
}

void passport_output_reset(void) {}

int32_t passport_output_push(int32_t ch)
{
    return ch >= 0 && ch <= 127;
}

int32_t passport_dictionary_count(void)
{
    return 2;
}

int32_t passport_dictionary_length(int32_t index)
{
    return index >= 0 && index < 2 ? 5 : -1;
}

int32_t passport_dictionary_char(int32_t index, int32_t offset)
{
    static const char *const words[] = {"apple", "berry"};
    if (index < 0 || index >= 2 || offset < 0 || offset >= 5) return -1;
    return (unsigned char)words[index][offset];
}
