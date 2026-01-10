#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arch/zx.h>
#include <input.h>
#include <intrinsic.h>

// Screen dimensions in characters
#define SCREEN_W 32
#define SCREEN_H 24
#define ATTR_START 22528

// Colors for attributes (Paper << 3 | Ink)
#define COL_BLACK   (0 << 3 | 0)
#define COL_PLAYER  (0 << 3 | 4) // Green
#define COL_INVADER (0 << 3 | 2) // Red
#define COL_BULLET  (0 << 3 | 6) // Yellow

// Game State
int playerX = 15;
int score = 0;
int gameOver = 0;

// Bullet state
int bulletX = -1;
int bulletY = -1;

// Invader state
#define INVADER_COLS 8
#define INVADER_ROWS 3
typedef struct {
    int x, y;
    int alive;
} Invader;

Invader invaders[INVADER_COLS * INVADER_ROWS];
int invaderDir = 1; 
int invaderTimer = 0;
int invaderSpeed = 6; // Higher is slower

// UDGs for Ship (A), Invader (B), Bullet (C)
const unsigned char udg_data[] = {
    0x18, 0x3C, 0x3C, 0x7E, 0xFF, 0xFF, 0x66, 0x00, // A: Player
    0x99, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x99, // B: Invader
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, // C: Bullet
};

// UDG character codes on the Spectrum are 144-164 (A-U).
#define UDG_BASE 144
#define UDG_PLAYER  (UDG_BASE + 0)
#define UDG_INVADER (UDG_BASE + 1)
#define UDG_BULLET  (UDG_BASE + 2)

void set_color(int x, int y, unsigned char attr) {
    unsigned char *attr_mem = (unsigned char *)ATTR_START;
    if (x >= 0 && x < 32 && y >= 0 && y < 24) {
        attr_mem[y * 32 + x] = attr;
    }
}

void draw_char(int x, int y, unsigned char c, unsigned char attr) {
    printf("\x16%c%c%c", (char)y, (char)x, (char)c);
    set_color(x, y, attr);
}

void clear_char(int x, int y) {
    printf("\x16%c%c ", (char)y, (char)x);
    set_color(x, y, COL_BLACK);
}

void init_game(void) {
    int i, j;
    void *udg_ptr = (void *)65368;
    memcpy(udg_ptr, udg_data, sizeof(udg_data));

    printf("%c", 12); // Clear screen
    zx_border(0);

    printf("\x16\x05\x08 SPACE INVADERS");
    printf("\x16\x08\x05 O/P: MOVE, SPACE: FIRE");
    printf("\x16\x0B\x06 PRESS SPACE TO START");
    
    // Wait for Space
    while (!in_key_pressed(IN_KEY_SCANCODE_SPACE)) {
        // Busy wait
    }
    in_wait_nokey();
    printf("%c", 12); 

    for (i = 0; i < INVADER_ROWS; i++) {
        for (j = 0; j < INVADER_COLS; j++) {
            int idx = i * INVADER_COLS + j;
            invaders[idx].x = j * 3 + 4;
            invaders[idx].y = i * 2 + 2;
            invaders[idx].alive = 1;
        }
    }
}

void update_game(void) {
    int i;
    
    // 1. Bullet Update
    if (bulletY >= 0) {
        clear_char(bulletX, bulletY);
        bulletY--;
        if (bulletY < 1) {
            bulletY = -1;
        } else {
            for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
                if (invaders[i].alive && bulletX == invaders[i].x && bulletY == invaders[i].y) {
                    clear_char(invaders[i].x, invaders[i].y);
                    invaders[i].alive = 0;
                    bulletY = -1;
                    score += 10;
                    break;
                }
            }
        }
    }

    // 2. Invader Movement
    invaderTimer++;
    if (invaderTimer >= invaderSpeed) {
        invaderTimer = 0;
        int hitEdge = 0;

        for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
            if (invaders[i].alive) {
                if ((invaderDir == 1 && invaders[i].x >= 30) || (invaderDir == -1 && invaders[i].x <= 1)) {
                    hitEdge = 1;
                    break;
                }
            }
        }

        if (hitEdge) {
            invaderDir *= -1;
            for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
                if (invaders[i].alive) {
                    clear_char(invaders[i].x, invaders[i].y);
                    invaders[i].y++;
                    if (invaders[i].y >= 21) gameOver = 1;
                }
            }
        } else {
            for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
                if (invaders[i].alive) {
                    clear_char(invaders[i].x, invaders[i].y);
                    invaders[i].x += invaderDir;
                }
            }
        }
    }
}

void main(void) {
    init_game();

    while (!gameOver) {
        int i, count;

        // Input Handling
        if (in_key_pressed(IN_KEY_SCANCODE_o) && playerX > 0) {
            clear_char(playerX, 22);
            playerX--;
        }
        if (in_key_pressed(IN_KEY_SCANCODE_p) && playerX < 31) {
            clear_char(playerX, 22);
            playerX++;
        }
        if (in_key_pressed(IN_KEY_SCANCODE_SPACE) && bulletY < 0) {
            bulletX = playerX;
            bulletY = 21;
        }

        update_game();

        // Rendering
        printf("\x16\x00\x00\x0F\x07 SCORE: %d", score);
        draw_char(playerX, 22, UDG_PLAYER, COL_PLAYER);
        if (bulletY >= 0) draw_char(bulletX, bulletY, UDG_BULLET, COL_BULLET);
        
        count = 0;
        for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
            if (invaders[i].alive) {
                draw_char(invaders[i].x, invaders[i].y, UDG_INVADER, COL_INVADER);
                count++;
            }
        }

        if (count == 0) {
            printf("\x16\x0C\x0A YOU WIN!");
            break;
        }

        // Use multiple halts to slow things down slightly without blocking
        intrinsic_halt();
        intrinsic_halt();
    }

    if (gameOver) printf("\x16\x0C\x09 GAME OVER!");
    while(1);
}
