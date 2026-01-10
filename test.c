#include <stdio.h>
#include <arch/zx.h>

void main(void) {
    printf("%c", 12); // Clear screen
    zx_border(1);     // Set border to blue
    printf("\x16\x05\x05Hello, World!");
    while (1) {}
}
