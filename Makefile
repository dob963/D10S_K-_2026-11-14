CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic

.PHONY: test clean

test: build/test_spectrum_detect
	./build/test_spectrum_detect

build/test_spectrum_detect: src/spectrum/spectrum_detect.c src/spectrum/spectrum_plan.c src/spectrum/spectrum_scanner.c src/ui/spectrum_view.c src/platform/mono128x64.c tests/test_spectrum_detect.c src/spectrum/spectrum_detect.h src/spectrum/spectrum_plan.h src/spectrum/spectrum_scanner.h src/ui/spectrum_view.h src/platform/mono128x64.h
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc/spectrum -Isrc/ui -Isrc/platform src/spectrum/spectrum_detect.c src/spectrum/spectrum_plan.c src/spectrum/spectrum_scanner.c src/ui/spectrum_view.c tests/test_spectrum_detect.c -o $@

clean:
	rm -rf build
