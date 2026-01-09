# Docker Z88DK Settings
DOCKER_IMAGE = z88dk/z88dk
ZCC = docker run --rm -v "$(shell pwd)":/src $(DOCKER_IMAGE) zcc
# -SO3 for optimization, -lm just in case you use math
FLAGS = +zx -vn -startup=1 -clib=sdcc_iy -create-app -SO3 -lm

# Pattern Rule: This builds any .tap from a matching .c file
%.tap: %.c
	@echo "[Pipeline] Compiling $< into $@..."
	$(ZCC) $(FLAGS) /src/$< -o /src/$(basename $@)
	@echo "[Pipeline] Fixing permissions..."
	sudo chown $(shell id -u):$(shell id -g) $@ $(basename $@)_CODE.bin
	@echo "--------------------------------------------------------"
	@echo "BUILD SUCCESSFUL: $@"
	@echo "To run: fbzx $@"
	@echo "--------------------------------------------------------"

# Shortcut to run a specific file: e.g., 'make run-plasma'
run-%: %.tap
	fbzx $<

clean:
	@echo "[Pipeline] Cleaning all binaries..."
	rm -f *.tap *_CODE.bin test plasma
