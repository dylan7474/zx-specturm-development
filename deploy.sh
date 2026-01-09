#!/bin/bash
set -euo pipefail

SCRIPT_VERSION="1.2.1"
project_dir="$HOME/src/zx_project"

install_fuse() {
  if ! command -v yay >/dev/null 2>&1; then
    echo "Error: yay not found. Please install yay first, then re-run this script." >&2
    exit 1
  fi

  if pacman -Q fuse-emulator >/dev/null 2>&1 || pacman -Q libspectrum >/dev/null 2>&1; then
    echo "Removing existing fuse-emulator/libspectrum packages to force a clean AUR rebuild..."
    sudo pacman -Rns --noconfirm fuse-emulator libspectrum
  fi

  echo "Installing Fuse emulator and libspectrum via yay (AUR rebuild)..."
  yay -S --aur --rebuild fuse-emulator libspectrum
}

install_docker() {
  if ! command -v docker >/dev/null 2>&1; then
    echo "Installing Docker..."
    sudo pacman -S docker
  fi

  echo "Enabling and starting Docker service..."
  sudo systemctl enable --now docker

  if ! id -nG "$USER" | grep -qw docker; then
    echo "Adding $USER to docker group..."
    sudo usermod -aG docker "$USER"
    echo "Note: Log out and back in (or run 'newgrp docker') for group changes to take effect."
  fi
}

create_project() {
  echo "Creating project directory at $project_dir..."
  mkdir -p "$project_dir"
  cd "$project_dir"

  cat <<'SCRIPT' > build.sh
#!/bin/bash
# 1. Run the compiler inside Docker
# 2. Map current folder to /src inside the container
# 3. Use the SDCC highly-optimized compiler
docker run --rm -v "$(pwd)":/src -it z88dk/z88dk \
  zcc +zx -vn -startup=1 -clib=sdcc_iy /src/test.c -o /src/test -create-app

# 4. Fix permissions (Docker output is owned by root by default)
sudo chown "$USER":"$USER" test.tap test_CODE.bin
SCRIPT
  chmod +x build.sh

  cat <<'CFILE' > test.c
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
CFILE
}

build_project() {
  cd "$project_dir"
  if command -v docker >/dev/null 2>&1; then
    echo "Building project with Docker..."
    ./build.sh

    if [ -f test.tap ]; then
      echo "Build complete: test.tap created."
    else
      echo "Warning: test.tap not found after build." >&2
    fi
  else
    echo "Warning: Docker is not available. Skipping build." >&2
  fi
}

echo "deploy.sh version: $SCRIPT_VERSION"

install_fuse
install_docker
create_project
build_project

echo "Setup complete. To run the emulator, execute: fuse $project_dir/test.tap"
