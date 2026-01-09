#include <stdio.h>
#include <arch/zx.h>
#include <string.h>

void main(void) {
    printf("%c", 12);

    zx_border(1);

    printf("\x16\x05\x05Hello Speccy dev!");
    printf("\n\n    Building from Garuda...");

    memset((void*)22528, 14, 768);

    while (1) {
    }
}
