# ZX Spectrum Development on Arch/Garuda

This repo ships a one-shot deployment script plus build tooling for a modern C-based ZX Spectrum workflow: edit locally, compile in Docker (Z88DK), and run in the FBZX emulator.

## 1. Quick setup (recommended)

Run the deployment script from the repo root:

```bash
./deploy.sh
```

`deploy.sh` performs the following steps:

- Ensures `yay` or `paru` is available (AUR helper).
- Installs the **FBZX** emulator from the AUR.
- Installs and enables Docker.
- Adds your user to the `docker` group if needed.
- Creates an executable `build.sh` helper (and a starter `test.c` if missing).

> **Note:** If you were just added to the `docker` group, log out/back in (or run `newgrp docker`) before running the build.

## 2. Build and run

### Option A: Use the generated build script

```bash
./build.sh
```

The script compiles `test.c` in the `z88dk/z88dk` container, fixes output file permissions, and launches FBZX with the generated `test.tap`.

### Option B: Use the Makefile

```bash
make
make run
```

`make` builds `test.tap` and `test_CODE.bin` from `test.c`, and `make run` launches FBZX.

## 3. Example program (`test.c`)

The included sample demonstrates basic text output and keyboard-driven border color changes:

```c
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
```

## 4. Troubleshooting

### Docker permission errors

If you see a permission error from Docker, confirm you are in the `docker` group:

```bash
id -nG "$USER" | grep -qw docker && echo "docker group set"
```

Log out/back in (or run `newgrp docker`) if you just added the group.

### FBZX tape loading

When FBZX starts:

1. Press `J`, then `Symbol Shift + P` twice (LOAD "").
2. Press `Enter`.
3. Press `F6` to start the tape.
