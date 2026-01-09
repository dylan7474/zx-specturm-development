#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$project_root"

# 1. Synchronize system to avoid partial upgrade states
echo "[deploy] Synchronizing system database..."
sudo pacman -Syu --noconfirm

# 2. Identify AUR helper (standard for Garuda/Arch)
yay_bin="$(command -v yay || command -v paru || true)"
if [ -z "$yay_bin" ]; then
  echo "[deploy] ERROR: yay or paru not found." >&2
  exit 1
fi

# 3. Install ZEsarUX instead of Fuse
# This avoids the libxml2 version mismatch and IgnorePkg warnings
if ! command -v zesarux &> /dev/null; then
  echo "[deploy] Installing ZEsarUX emulator from AUR..."
  "$yay_bin" -S --noconfirm zesarux
else
  echo "[deploy] ZEsarUX is already installed."
fi

# 4. Install and enable Docker for the toolchain
if ! command -v docker &> /dev/null; then
    echo "[deploy] Installing and enabling Docker..."
    sudo pacman -S --needed --noconfirm docker
    sudo systemctl enable --now docker
fi

# 5. Add user to docker group
if ! id -nG "$USER" | grep -qw docker; then
  echo "[deploy] Adding $USER to docker group"
  sudo usermod -aG docker "$USER"
fi

# 6. Generate the build script
# Modified to launch ZEsarUX automatically after compilation
cat <<'BUILD' > "$project_root/build.sh"
#!/usr/bin/env bash
set -euo pipefail

# Run the Z88DK compiler inside Docker
sg docker -c "docker run --rm -v \"$(pwd)\":/src z88dk/z88dk \
  zcc +zx -vn -startup=1 -clib=sdcc_iy /src/test.c -o /src/test -create-app"

# Fix permissions for the output files
sudo chown "$USER":"$USER" test.tap test_CODE.bin

echo "[build] Success. Launching ZEsarUX..."
zesarux --quickexit --tape test.tap
BUILD
chmod +x "$project_root/build.sh"

# 7. Generate example hardware probe code if missing
if [ ! -f "$project_root/test.c" ]; then
  cat <<'CODE' > "$project_root/test.c"
#include <stdio.h>
#include <arch/zx.h>
#include <string.h>

void main(void) {
    // Clear screen using ROM routine
    printf("%c", 12);

    // Set border to blue
    zx_border(1);

    printf("\x16\x05\x05Hello Speccy dev!");
    printf("\n\n    Building for ZEsarUX...");

    // Direct memory access to attribute memory
    memset((void*)22528, 14, 768);

    while (1) {
        // Prevent crash
    }
}
CODE
fi

echo "[deploy] Setup complete. Run ./build.sh to compile and test."
