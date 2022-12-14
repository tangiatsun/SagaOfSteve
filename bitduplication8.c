#include <stdio.h>

// original
// uint32_t interleave_4(uint8_t value)
// {
//     uint32_t x = value;
//     x = (x | (x << 12)) /* & 0x000F000F */; // GCC is not able to remove redundant & here
//     x = (x | (x <<  6)) & 0x03030303;
//     x = (x | (x <<  3)) & 0x11111111;
//     x = (x << 4) - x;
//     return x;
// }

// 8 TO 64 VERSION
// value= input (8bits)
// x= output (64bits)
long long interleave_8(char value)
{
    long long x = value;
    x = (x | (x << 28)) /* & 0xF0000000F */; // GCC is not able to remove redundant & here
    x = (x | (x <<  14)) & 0x3000300030003;
    x = (x | (x <<  7)) & 0x101010101010101;
    x = (x << 8) - x;
    return x;
}

// int main()
// {
//     printf(" %x", interleave(0x22));

// }