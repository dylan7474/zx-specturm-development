#include <stdio.h>
#include <string.h>
#include <arch/zx.h>
#include <input.h>

#define SCREEN_BASE 16384
#define ATTR_BASE 22528
#define SCREEN_WIDTH 32
#define SCREEN_HEIGHT 24

#define INVADER_COLS 6
#define INVADER_ROWS 3
#define INVADER_SPACING 3
#define INVADER_WIDTH 2

#define PLAYER_Y 21
#define PLAYER_WIDTH 2

static const unsigned char sin_table[32] = {
    4, 5, 5, 6, 7, 7, 7, 7, 6, 6, 5, 4, 3, 2, 2, 1,
    0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 5, 6, 7, 7, 7, 7
};

static const unsigned char sprite_player[8][2] = {
    { 0x00, 0x00 },
    { 0x18, 0x18 },
    { 0x3C, 0x3C },
    { 0x7E, 0x7E },
    { 0xDB, 0xDB },
    { 0xFF, 0xFF },
    { 0x24, 0x24 },
    { 0x42, 0x42 }
};

static const unsigned char sprite_invader_a[8][2] = {
    { 0x3C, 0x3C },
    { 0x7E, 0x7E },
    { 0xDB, 0xDB },
    { 0xFF, 0xFF },
    { 0xBD, 0xBD },
    { 0x24, 0x24 },
    { 0x42, 0x42 },
    { 0x81, 0x81 }
};

static const unsigned char sprite_invader_b[8][2] = {
    { 0x3C, 0x3C },
    { 0x7E, 0x7E },
    { 0xDB, 0xDB },
    { 0xFF, 0xFF },
    { 0x7E, 0x7E },
    { 0x24, 0x24 },
    { 0x5A, 0x5A },
    { 0xA5, 0xA5 }
};

static const unsigned char sprite_bullet[8] = {
    0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00
};

static const unsigned char font_v[8] = { 0x81, 0x81, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18 };
static const unsigned char font_a[8] = { 0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00 };
static const unsigned char font_d[8] = { 0x7C, 0x42, 0x41, 0x41, 0x41, 0x42, 0x7C, 0x00 };
static const unsigned char font_e[8] = { 0x7E, 0x40, 0x7C, 0x40, 0x40, 0x40, 0x7E, 0x00 };
static const unsigned char font_r[8] = { 0x7C, 0x42, 0x42, 0x7C, 0x50, 0x48, 0x44, 0x00 };
static const unsigned char font_s[8] = { 0x3C, 0x40, 0x3C, 0x02, 0x02, 0x42, 0x3C, 0x00 };
static const unsigned char font_c[8] = { 0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0x00 };
static const unsigned char font_o[8] = { 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00 };
static const unsigned char font_blank[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const unsigned char font_digits[10][8] = {
    { 0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00 },
    { 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00 },
    { 0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00 },
    { 0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00 },
    { 0x0C, 0x1C, 0x2C, 0x4C, 0x7E, 0x0C, 0x0C, 0x00 },
    { 0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00 },
    { 0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00 },
    { 0x7E, 0x66, 0x0C, 0x18, 0x18, 0x18, 0x18, 0x00 },
    { 0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00 },
    { 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00 }
};

static unsigned char *zx_screen_address(unsigned char x, unsigned char y_pixel) {
    unsigned int offset;

    offset = ((y_pixel & 0xC0) << 5)
           | ((y_pixel & 0x38) << 2)
           | ((y_pixel & 0x07) << 8)
           | x;

    return (unsigned char *)(SCREEN_BASE + offset);
}

static void draw_sprite(unsigned char x, unsigned char y_char, const unsigned char *sprite) {
    unsigned char row;
    unsigned char y_pixel = y_char << 3;

    for (row = 0; row < 8; ++row) {
        unsigned char *cell = zx_screen_address(x, y_pixel + row);
        *cell = sprite[row];
    }
}

static void draw_sprite16(unsigned char x, unsigned char y_char, const unsigned char sprite[8][2]) {
    unsigned char row;
    unsigned char y_pixel = y_char << 3;

    for (row = 0; row < 8; ++row) {
        unsigned char *cell = zx_screen_address(x, y_pixel + row);
        cell[0] = sprite[row][0];
        cell[1] = sprite[row][1];
    }
}

static void draw_attribute(unsigned char x, unsigned char y, unsigned char ink, unsigned char paper) {
    unsigned char *attr_mem = (unsigned char *)ATTR_BASE;
    attr_mem[y * SCREEN_WIDTH + x] = (paper << 3) | (ink & 0x07);
}

static void draw_attribute_pair(unsigned char x, unsigned char y, unsigned char ink, unsigned char paper) {
    draw_attribute(x, y, ink, paper);
    draw_attribute(x + 1, y, ink, paper);
}

static void draw_font_char(unsigned char x, unsigned char y, const unsigned char *glyph, unsigned char ink) {
    unsigned char row;
    unsigned char y_pixel = y << 3;

    for (row = 0; row < 8; ++row) {
        unsigned char *cell = zx_screen_address(x, y_pixel + row);
        *cell = glyph[row];
    }

    draw_attribute(x, y, ink, 0);
}

static void draw_word(unsigned char x, unsigned char y, const char *word, unsigned char ink) {
    unsigned char i;

    for (i = 0; word[i] != '\0'; ++i) {
        const unsigned char *glyph = NULL;
        switch (word[i]) {
        case 'V': glyph = font_v; break;
        case 'A': glyph = font_a; break;
        case 'D': glyph = font_d; break;
        case 'E': glyph = font_e; break;
        case 'R': glyph = font_r; break;
        case 'S': glyph = font_s; break;
        case 'C': glyph = font_c; break;
        case 'O': glyph = font_o; break;
        default: glyph = font_blank; break;
        }
        draw_font_char(x + i, y, glyph, ink);
    }
}

static void draw_digit(unsigned char x, unsigned char y, unsigned char digit, unsigned char ink) {
    if (digit > 9) {
        draw_font_char(x, y, font_blank, ink);
        return;
    }

    draw_font_char(x, y, font_digits[digit], ink);
}

static void clear_playfield(void) {
    unsigned char x, y, row;

    for (y = 2; y < SCREEN_HEIGHT; ++y) {
        for (x = 0; x < SCREEN_WIDTH; ++x) {
            for (row = 0; row < 8; ++row) {
                unsigned char *cell = zx_screen_address(x, (y << 3) + row);
                *cell = 0x00;
            }
            draw_attribute(x, y, 0, 0);
        }
    }
}

static void draw_score(unsigned int score) {
    unsigned char thousands = (score / 1000) % 10;
    unsigned char hundreds = (score / 100) % 10;
    unsigned char tens = (score / 10) % 10;
    unsigned char ones = score % 10;

    draw_word(1, 0, "SCORE", 6);
    draw_digit(8, 0, thousands, 6);
    draw_digit(9, 0, hundreds, 6);
    draw_digit(10, 0, tens, 6);
    draw_digit(11, 0, ones, 6);
}

int main(void) {
    unsigned char frame = 0;
    unsigned char invader_offset_x = 4;
    unsigned char invader_offset_y = 6;
    signed char invader_dir = 1;
    unsigned char player_x = 15;
    unsigned char bullet_active = 0;
    unsigned char bullet_x = 0;
    unsigned char bullet_y = 0;
    unsigned int score = 0;
    unsigned char invader_alive[INVADER_ROWS][INVADER_COLS];
    unsigned char row, col;

    printf("%c", 12);
    zx_border(0);
    memset(invader_alive, 1, sizeof(invader_alive));

    draw_word(10, 1, "VADERS", 2);
    draw_score(score);

    while (1) {
        unsigned char key = in_inkey();

        if ((key == 'o' || key == 'O') && player_x > 0) {
            player_x--;
        }
        if ((key == 'p' || key == 'P') && player_x < (SCREEN_WIDTH - PLAYER_WIDTH)) {
            player_x++;
        }
        if ((key == ' ') && !bullet_active) {
            bullet_active = 1;
            bullet_x = player_x + 1;
            bullet_y = PLAYER_Y - 1;
        }

        if ((frame & 7) == 0) {
            invader_offset_x += invader_dir;
            if (invader_offset_x == 1
                || invader_offset_x == (SCREEN_WIDTH - INVADER_WIDTH - ((INVADER_COLS - 1) * INVADER_SPACING))) {
                invader_dir = -invader_dir;
                invader_offset_y++;
            }
        }

        if (bullet_active) {
            if (bullet_y == 0) {
                bullet_active = 0;
            } else {
                bullet_y--;
            }
        }

        for (row = 0; row < INVADER_ROWS; ++row) {
            for (col = 0; col < INVADER_COLS; ++col) {
                if (!invader_alive[row][col]) {
                    continue;
                }

                if (bullet_active
                    && bullet_y == invader_offset_y + (row * 2)
                    && (bullet_x == invader_offset_x + (col * INVADER_SPACING)
                        || bullet_x == invader_offset_x + (col * INVADER_SPACING) + 1)) {
                    invader_alive[row][col] = 0;
                    bullet_active = 0;
                    score += 10;
                    draw_score(score);
                }
            }
        }

        clear_playfield();

        draw_sprite16(player_x, PLAYER_Y, sprite_player);
        draw_attribute_pair(player_x, PLAYER_Y, 7, 0);

        if (bullet_active) {
            draw_sprite(bullet_x, bullet_y, sprite_bullet);
            draw_attribute(bullet_x, bullet_y, 6, 0);
        }

        for (row = 0; row < INVADER_ROWS; ++row) {
            for (col = 0; col < INVADER_COLS; ++col) {
                if (!invader_alive[row][col]) {
                    continue;
                }

                if ((frame & 8) == 0) {
                    draw_sprite16(invader_offset_x + (col * INVADER_SPACING),
                                  invader_offset_y + (row * 2),
                                  sprite_invader_a);
                } else {
                    draw_sprite16(invader_offset_x + (col * INVADER_SPACING),
                                  invader_offset_y + (row * 2),
                                  sprite_invader_b);
                }

                draw_attribute_pair(invader_offset_x + (col * INVADER_SPACING),
                                    invader_offset_y + (row * 2),
                                    sin_table[(frame + col * 3 + row * 5) & 31],
                                    0);
            }
        }

        frame++;
    }
}
