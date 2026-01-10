#include <stdio.h>
#include <string.h>
#include <arch/zx.h>
#include <input.h>
#include <intrinsic.h>

#define SCREEN_W 32
#define SCREEN_H 24
#define ATTR_START 22528

#define COL_BLACK   (0 << 3 | 0)
#define COL_PLAYER  (0 << 3 | 4)
#define COL_INVADER (0 << 3 | 2)
#define COL_BULLET  (0 << 3 | 6)
#define UDG_BASE 144
#define UDG_PLAYER  (UDG_BASE + 0)
#define UDG_INVADER (UDG_BASE + 1)
#define UDG_BULLET  (UDG_BASE + 2)

#define INVADER_COLS 8
#define INVADER_ROWS 3

#define PLAYER_Y 22
#define INVADER_STEP_DELAY 6
#define BULLET_START_Y 21

typedef struct {
    int x;
    int y;
    int alive;
} Invader;

typedef struct {
    int x;
    int y;
    int active;
} Bullet;

static Invader invaders[INVADER_COLS * INVADER_ROWS];
static Bullet playerBullet;

static int playerX = 15;
static int invaderDir = 1;
static int invaderTimer = 0;
static int score = 0;
static int gameOver = 0;

static const unsigned char udg_data[] = {
    0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24, 0x5A, 0xA5, // A: Player ship
    0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0x7E, 0x24, 0x42, // B: Invader
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18  // C: Bullet
};

static void set_color(int x, int y, unsigned char attr) {
    unsigned char *attr_mem = (unsigned char *)ATTR_START;
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        attr_mem[y * SCREEN_W + x] = attr;
    }
}

static void draw_char(int x, int y, unsigned char c, unsigned char attr) {
    printf("\x16%c%c%c", (char)y, (char)x, (char)c);
    set_color(x, y, attr);
}

static void clear_char(int x, int y) {
    printf("\x16%c%c ", (char)y, (char)x);
    set_color(x, y, COL_BLACK);
}

static void draw_status(void) {
    printf("\x16\x00\x00\x0F\x07 SCORE: %d", score);
}

static void init_udgs(void) {
    void *udg_ptr = (void *)65368;
    memcpy(udg_ptr, udg_data, sizeof(udg_data));
}

static void reset_invaders(void) {
    int i, j;
    for (i = 0; i < INVADER_ROWS; i++) {
        for (j = 0; j < INVADER_COLS; j++) {
            int idx = i * INVADER_COLS + j;
            invaders[idx].x = j * 3 + 3;
            invaders[idx].y = i * 2 + 3;
            invaders[idx].alive = 1;
        }
    }
}

static void draw_invaders(void) {
    int i;
    for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
        if (invaders[i].alive) {
            draw_char(invaders[i].x, invaders[i].y, UDG_INVADER, COL_INVADER);
        }
    }
}

static void clear_invaders(void) {
    int i;
    for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
        if (invaders[i].alive) {
            clear_char(invaders[i].x, invaders[i].y);
        }
    }
}

static int remaining_invaders(void) {
    int i;
    int count = 0;
    for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
        if (invaders[i].alive) {
            count++;
        }
    }
    return count;
}

static void init_game(void) {
    init_udgs();
    printf("%c", 12);
    zx_border(0);

    printf("\x16\x05\x08 SPACE INVADERS");
    printf("\x16\x08\x04 O/P: MOVE  SPACE: FIRE");
    printf("\x16\x0B\x06 PRESS SPACE TO START");

    while (!in_key_pressed(IN_KEY_SCANCODE_SPACE)) {
        intrinsic_halt();
    }
    in_wait_nokey();

    printf("%c", 12);

    playerX = 15;
    score = 0;
    gameOver = 0;
    invaderDir = 1;
    invaderTimer = 0;
    playerBullet.active = 0;

    reset_invaders();
}

static void update_player(void) {
    if (in_key_pressed(IN_KEY_SCANCODE_o) && playerX > 0) {
        clear_char(playerX, PLAYER_Y);
        playerX--;
    }
    if (in_key_pressed(IN_KEY_SCANCODE_p) && playerX < SCREEN_W - 1) {
        clear_char(playerX, PLAYER_Y);
        playerX++;
    }
    if (in_key_pressed(IN_KEY_SCANCODE_SPACE) && !playerBullet.active) {
        playerBullet.active = 1;
        playerBullet.x = playerX;
        playerBullet.y = BULLET_START_Y;
    }
}

static void update_bullet(void) {
    int i;
    if (!playerBullet.active) {
        return;
    }

    clear_char(playerBullet.x, playerBullet.y);
    playerBullet.y--;

    if (playerBullet.y < 1) {
        playerBullet.active = 0;
        return;
    }

    for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
        if (invaders[i].alive && playerBullet.x == invaders[i].x && playerBullet.y == invaders[i].y) {
            clear_char(invaders[i].x, invaders[i].y);
            invaders[i].alive = 0;
            playerBullet.active = 0;
            score += 10;
            break;
        }
    }
}

static void update_invaders(void) {
    int i;
    int hitEdge = 0;

    invaderTimer++;
    if (invaderTimer < INVADER_STEP_DELAY) {
        return;
    }
    invaderTimer = 0;

    for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
        if (!invaders[i].alive) {
            continue;
        }
        if ((invaderDir > 0 && invaders[i].x >= SCREEN_W - 2) ||
            (invaderDir < 0 && invaders[i].x <= 1)) {
            hitEdge = 1;
            break;
        }
    }

    clear_invaders();

    if (hitEdge) {
        invaderDir = -invaderDir;
        for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
            if (!invaders[i].alive) {
                continue;
            }
            invaders[i].y++;
            if (invaders[i].y >= PLAYER_Y) {
                gameOver = 1;
            }
        }
    } else {
        for (i = 0; i < INVADER_COLS * INVADER_ROWS; i++) {
            if (invaders[i].alive) {
                invaders[i].x += invaderDir;
            }
        }
    }

    draw_invaders();
}

static void render_frame(void) {
    draw_status();
    draw_char(playerX, PLAYER_Y, UDG_PLAYER, COL_PLAYER);
    if (playerBullet.active) {
        draw_char(playerBullet.x, playerBullet.y, UDG_BULLET, COL_BULLET);
    }
}

void main(void) {
    init_game();
    draw_invaders();

    while (!gameOver) {
        update_player();
        update_bullet();
        update_invaders();
        render_frame();

        if (remaining_invaders() == 0) {
            printf("\x16\x0C\x0A YOU WIN!");
            break;
        }

        intrinsic_halt();
        intrinsic_halt();
    }

    if (gameOver) {
        printf("\x16\x0C\x09 GAME OVER!");
    }

    while (1) {
    }
}
