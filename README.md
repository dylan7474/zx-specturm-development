# Tutorial: ZX Spectrum Development on Modern Linux (Garuda/Arch)

This guide provides a complete walkthrough for setting up a modern C-based development environment for the ZX Spectrum. We use a cross-development approach: write code in a modern editor, compile via Docker, and test in a dedicated emulator.

## 1. Environment setup

### A. The emulator (Fuse)

On Garuda/Arch, pre-compiled binaries can suffer from library version mismatches (for example, `libxml2.so.2` missing). To fix this, install from the AUR and rebuild locally so the binaries match your system libraries:

```bash
# Rebuild from source to ensure library compatibility
yay -S --aur --rebuild fuse-emulator libspectrum
```

> **Note:** The deploy script removes any existing `fuse-emulator`/`libspectrum` packages and then runs the exact command above. Ensure `yay` is installed before running it.

### B. The compiler (Z88DK via Docker)

Z88DK is the standard C compiler for Z80 systems. Installing it natively on Arch can be difficult due to dependencies, so we use Docker for a clean, reproducible toolchain.

```bash
# Install and start Docker
sudo pacman -S docker
sudo systemctl enable --now docker

# Add yourself to the docker group so you don't need 'sudo' for every build
sudo usermod -aG docker "$USER"
```

> **Note:** Log out and back in (or run `newgrp docker`) for the group change to take effect.

## 2. Project configuration

Create a dedicated folder for your Spectrum projects:

```bash
mkdir -p ~/src/zx_project
cd ~/src/zx_project
```

When you run `deploy.sh`, it prints a version banner so you can confirm which script revision you're using.

### Create the build script

To avoid typing a long command every time, create a `build.sh` script:

```bash
nano build.sh
```

Paste the following:

```bash
#!/bin/bash
# 1. Run the compiler inside Docker
# 2. Map current folder to /src inside the container
# 3. Use the SDCC highly-optimized compiler
docker run --rm -v "$(pwd)":/src -it z88dk/z88dk \
  zcc +zx -vn -startup=1 -clib=sdcc_iy /src/test.c -o /src/test -create-app

# 4. Fix permissions (Docker output is owned by root by default)
sudo chown "$USER":"$USER" test.tap test_CODE.bin
```

Give it execution permissions:

```bash
chmod +x build.sh
```

## 3. Example code: “The hardware probe”

Create `test.c`. This code demonstrates the three pillars of Spectrum development: ROM calls (printf), hardware ports (border), and direct memory access (VRAM).

```c
#include <stdio.h>
#include <arch/zx.h>
#include <string.h>

void main(void) {
    // 1. Clear screen using the Spectrum's internal ROM routine
    printf("%c", 12);

    // 2. Manipulate the ULA (hardware port) to change border color
    zx_border(1); // Set border to blue

    // 3. Print text using standard C
    printf("\x16\x05\x05Hello Speccy dev!");
    printf("\n\n    Building from Garuda...");

    // 4. Direct memory access (DMA)
    // Writing to address 22528 (attribute/color memory)
    // Formula: (paper * 8) + ink.
    // Here: (1 * 8) + 6 = blue background, yellow text.
    memset((void*)22528, 14, 768);

    while (1) {
        // Infinite loop to prevent the Speccy from crashing/resetting
    }
}
```

## 4. The development loop

1. **Write/Edit:** Open `test.c` in your favorite editor.
2. **Compile:** Run `./build.sh` in your terminal.
3. **Check:** Ensure `test.tap` was created.
4. **Run:** Execute `fuse test.tap`.

## 5. Troubleshooting

### Fuse fails to start due to `libxml2.so.2`

If you see:

```
fuse: error while loading shared libraries: libxml2.so.2: cannot open shared object file: No such file or directory
```

Rebuild Fuse and libspectrum from AUR so they link against your system libraries:

```bash
sudo pacman -Rns fuse-emulator libspectrum
yay -S --aur --rebuild fuse-emulator libspectrum
```

### “File not found” in Docker

Ensure you are running the `build.sh` script from the same directory where `test.c` lives. Docker only sees your files through the volume mapping (`-v "$(pwd)":/src`).

### “Attribute clash”

If you see colors bleeding into 8×8 squares, remember: the Spectrum can only have two colors per 8×8 pixel block. This is a hardware limitation, not a bug in your code.

### Screen memory layout

The Spectrum screen is interleaved. If you write to memory linearly, it will fill the screen in three distinct “shuffled” chunks. This is normal.

## 6. Next steps

- **Graphics:** Look into Multipaint for Linux to draw screens.
- **Libraries:** Explore `sp1.h` in Z88DK for high-performance sprites.

## 7. Cleanup (undo a deployment)

If you need to revert the setup performed by `deploy.sh`, run the following commands:

```bash
# Remove the project directory
rm -rf ~/src/zx_project

# Remove Fuse emulator and libspectrum
yay -Rns fuse-emulator libspectrum

# Remove Docker (optional)
sudo pacman -Rns docker
```
