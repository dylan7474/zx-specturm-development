# Project Settings
PROJECT = test
SOURCES = test.c
TAPE    = $(PROJECT).tap

# Docker Z88DK Settings
DOCKER_IMAGE = z88dk/z88dk
ZCC = docker run --rm -v "$(shell pwd)":/src $(DOCKER_IMAGE) zcc
FLAGS = +zx -vn -startup=1 -clib=sdcc_iy -create-app

.PHONY: all clean run

all: $(TAPE)

$(TAPE): $(SOURCES)
	@echo "[Pipeline] Compiling $(SOURCES)..."
	$(ZCC) $(FLAGS) /src/$(SOURCES) -o /src/$(PROJECT)
	@echo "[Pipeline] Fixing permissions..."
	sudo chown $(shell id -u):$(shell id -g) $(TAPE) $(PROJECT)_CODE.bin
	@echo "--------------------------------------------------------"
	@echo "BUILD SUCCESSFUL!"
	@echo "--------------------------------------------------------"
	@echo "To run in FBZX:"
	@echo " 1. Press 'J' then '""' (LOAD \"\")"
	@echo " 2. Press 'Enter'"
	@echo " 3. PRESS 'F6' TO START THE TAPE"
	@echo "--------------------------------------------------------"

run: all
	@echo "[Pipeline] Launching FBZX..."
	fbzx $(TAPE)

clean:
	@echo "[Pipeline] Cleaning up..."
	rm -f $(TAPE) $(PROJECT)_CODE.bin $(PROJECT)
