#include <stdio.h>
#include <arch/zx.h>
#include <string.h>
#include <input.h>

void main(void) {
    // 1. Setup the screen
    printf("%c", 12);               // Clear screen
    zx_border(1);                   // Set border to blue
    
    printf("\x16\x05\x05Advanced Speccy Dev");
    printf("\n\n    Press 'B' for Blue Border");
    printf("\n    Press 'R' for Red Border");
    printf("\n    Press 'G' for Green Border");
    
    // 2. Main Input Loop
    while (1) {
        if (in_key_pressed(IN_KEY_SCANCODE_b)) {
            zx_border(1); // Blue
        }
        if (in_key_pressed(IN_KEY_SCANCODE_r)) {
            zx_border(2); // Red
        }
        if (in_key_pressed(IN_KEY_SCANCODE_g)) {
            zx_border(4); // Green
        }
    }
}
