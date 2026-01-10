#!/usr/bin/env bash
set -euo pipefail

# Project root setup
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$project_root"

echo "[deploy] Synchronizing system and installing FBZX from AUR..."
yay_bin="$(command -v yay || command -v paru || true)"
if [ -z "$yay_bin" ]; then
  echo "[deploy] ERROR: yay or paru not found." >&2
  exit 1
fi

# Install FBZX
"$yay_bin" -S --aur --noconfirm fbzx

echo "[deploy] Configuring Docker environment..."
sudo pacman -S --needed --noconfirm docker
sudo systemctl enable --now docker

if ! id -nG "$USER" | grep -qw docker; then
  sudo usermod -aG docker "$USER"
fi

# Create the automated build script
cat <<'BUILD' > "$project_root/build.sh"
#!/usr/bin/env bash
set -euo pipefail

# 1. Compile via Z88DK Docker container
sg docker -c "docker run --rm -v \"$(pwd)\":/src z88dk/z88dk \
  zcc +zx -vn -startup=1 -clib=sdcc_iy /src/test.c -o /src/test -create-app"

# 2. Fix permissions
sudo chown "$USER":"$USER" test.tap test_CODE.bin

# 3. Success instructions
echo "--------------------------------------------------------"
echo "BUILD SUCCESSFUL!"
echo "--------------------------------------------------------"
echo "To run your code in FBZX:"
echo " 1. Press 'J' then 'Symbol Shift + P' twice (LOAD \"\")"
echo " 2. Press 'Enter'"
echo " 3. PRESS 'F6' TO START THE TAPE"
echo "--------------------------------------------------------"

fbzx test.tap
BUILD
chmod +x "$project_root/build.sh"

# Ensure test.c is ready
if [ ! -f "$project_root/test.c" ]; then
  cat <<'CODE' > "$project_root/test.c"
#include <stdio.h>
#include <arch/zx.h>

void main(void) {
    printf("%c", 12); // Clear screen
    zx_border(1);     // Set border to blue
    printf("\x16\x05\x05Hello, World!");
    while (1) {}
}
CODE
fi

echo "[deploy] Done. Run: ./build.sh"
