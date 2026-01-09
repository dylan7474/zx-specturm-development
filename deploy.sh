#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$project_root"

echo "[deploy] Installing Fuse emulator and libspectrum from AUR (rebuild)"
yay -S --aur --rebuild fuse-emulator libspectrum

echo "[deploy] Installing and enabling Docker"
sudo pacman -S --needed docker
sudo systemctl enable --now docker

if ! id -nG "$USER" | grep -qw docker; then
  echo "[deploy] Adding $USER to docker group"
  sudo usermod -aG docker "$USER"
  echo "[deploy] NOTE: Log out and back in (or run 'newgrp docker') for docker group changes to take effect."
fi

cat <<'BUILD' > "$project_root/build.sh"
#!/usr/bin/env bash
set -euo pipefail

docker run --rm -v "$(pwd)":/src z88dk/z88dk \
  zcc +zx -vn -startup=1 -clib=sdcc_iy /src/test.c -o /src/test -create-app

sudo chown "$USER":"$USER" test.tap test_CODE.bin
BUILD
chmod +x "$project_root/build.sh"

if [ ! -f "$project_root/test.c" ]; then
  cat <<'CODE' > "$project_root/test.c"
#include <stdio.h>
#include <arch/zx.h>
#include <string.h>

void main(void) {
    printf("%c", 12);

    zx_border(1);

    printf("\x16\x05\x05Hello Speccy dev!");
    printf("\n\n    Building from Garuda...");

    memset((void*)22528, 14, 768);

    while (1) {
    }
}
CODE
fi

echo "[deploy] Building example project"
"$project_root/build.sh"

echo "[deploy] Done. Run: fuse test.tap"
