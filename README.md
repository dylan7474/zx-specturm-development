This version is written as a comprehensive tutorial. It covers the "why" and "how" of every step we took, including the fixes for the library issues we encountered on Garuda.

Markdown

# Tutorial: ZX Spectrum Development on Modern Linux (Garuda/Arch)

This guide provides a complete walkthrough for setting up a modern C-based development environment for the ZX Spectrum. We use a "Cross-Development" approach: writing code in a modern editor, compiling via Docker, and testing in a specialized emulator.

---

## 1. Environment Setup

### A. The Emulator (Fuse)
On Garuda/Arch, pre-compiled binaries often suffer from "Library Version Mismatch" (e.g., `libxml2.so.2` not found). To fix this, we install from the AUR and force a local rebuild so the software matches your system exactly.

```bash
# Rebuild from source to ensure library compatibility
yay -S --aur --rebuild fuse-emulator libspectrum
B. The Compiler (Z88DK via Docker)
Z88DK is the standard C compiler for Z80 systems. Installing it natively on Arch can be difficult due to complex dependencies. We use Docker to provide a "clean room" environment that contains all necessary libraries (zx.lib, etc.).

Bash

# Install and start Docker
sudo pacman -S docker
sudo systemctl enable --now docker

# Add yourself to the docker group so you don't need 'sudo' for every build
sudo usermod -aG docker $USER
Note: You must log out and log back in for the group change to take effect.

2. Project Configuration
Create a dedicated folder for your Spectrum projects:

Bash

mkdir -p ~/src/zx_project && cd ~/src/zx_project
Create the Build Script
To avoid typing a long command every time, create a file named build.sh:

Bash

nano build.sh
Paste the following:

Bash

#!/bin/bash
# 1. Run the compiler inside Docker
# 2. Map current folder to /src inside the container
# 3. Use the SDCC highly-optimized compiler
docker run -v $(pwd):/src -it z88dk/z88dk zcc +zx -vn -startup=1 -clib=sdcc_iy /src/test.c -o /src/test -create-app

# 4. Fix permissions (Docker output is owned by root by default)
sudo chown $USER:$USER test.tap test_CODE.bin
Give it execution permissions:

Bash

chmod +x build.sh
3. Example Code: "The Hardware Probe"
Create test.c. This code demonstrates the three pillars of Speccy dev: ROM Calls (printf), Hardware Ports (border), and Direct Memory Access (VRAM).

C

#include <stdio.h>
#include <arch/zx.h>
#include <string.h>

void main(void) {
    // 1. Clear Screen using the Spectrum's internal ROM routine
    printf("%c", 12); 

    // 2. Manipulate the ULA (Hardware Port) to change border color
    zx_border(1); // Set border to Blue

    // 3. Print text using standard C
    printf("\x16\x05\x05Hello Speccy dev!");
    printf("\n\n    Building from Garuda...");

    // 4. Direct Memory Access (DMA)
    // Writing to address 22528 (Attribute/Color memory)
    // Formula: (Paper * 8) + Ink. 
    // Here: (1 * 8) + 6 = Blue background, Yellow text.
    memset((void*)22528, 14, 768); 

    while(1); // Infinite loop to prevent the Speccy from crashing/resetting
}
4. The Development Loop
Write/Edit: Open test.c in your favorite editor.

Compile: Run ./build.sh in your terminal.

Check: Ensure test.tap was created.

Run: Execute fuse test.tap.

5. Troubleshooting
"File not found" in Docker
Ensure you are running the build.sh script from the same directory where test.c lives. Docker sees your files through a "tunnel" created by the -v $(pwd):/src flag.

"Attribute Clash"
If you see colors "bleeding" into 8x8 squares, remember: The Spectrum can only have two colors per 8x8 pixel block. This is a hardware limitation, not a bug in your code!

Screen Memory Layout
The Spectrum screen is interleaved. If you write to memory linearly, it will fill the screen in three distinct "shuffled" chunks. This is normal.

6. Next Steps
Graphics: Look into Multipaint for Linux to draw screens.

Libraries: Explore cpwrs.h or sp1.h in Z88DK for high-performance sprites.
