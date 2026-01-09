#include <stdio.h>
#include <arch/zx.h>
#include <math.h>
#include <string.h>

// Pre-calculated sine table (values 0-7 for colors)
// This represents one full circle (0 to 2*PI) scaled for Spectrum colors
const unsigned char sin_table[32] = {
    4, 5, 5, 6, 7, 7, 7, 7, 6, 6, 5, 4, 3, 2, 2, 1,
    0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 5, 6, 7, 7, 7, 7
};

void main(void) {
    unsigned int x, y, frame = 0;
    unsigned char *attr_mem = (unsigned char *)22528; // Standard Attribute Memory

    printf("%c", 12); // Clear screen
    zx_border(0);     // Black border for contrast

    printf("\x16\x01\x01    ** PLASMA DEMO **");
    printf("\n\n    Hardware-accelerated logic");
    printf("\n    using Lookup Tables (LUT)");

    while (1) {
        for (y = 0; y < 24; ++y) {
            for (x = 0; x < 32; ++x) {
                // We use bitwise AND (& 31) to wrap around the 32-entry table
                // Combining two different offsets creates the "Plasma" ripple
                unsigned char i = (x + frame) & 31;
                unsigned char j = (y + (frame >> 1)) & 31;
                
                unsigned char color = (sin_table[i] + sin_table[j]) >> 1;
                
                // Construct Attribute: (Paper << 3) | Ink
                // We set Ink to 0 (Black) so the "Paper" color is all we see
                attr_mem[y * 32 + x] = (color << 3);
            }
        }
        frame++; // Advance the animation
    }
}
