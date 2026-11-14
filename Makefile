CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic

.PHONY: test clean

test: build/test_spectrum_detect
	./build/test_spectrum_detect

build/test_spectrum_detect: src/spectrum/spectrum_detect.c src/spectrum/spectrum_plan.c src/spectrum/spectrum_scanner.c tests/test_spectrum_detect.c src/spectrum/spectrum_detect.h src/spectrum/spectrum_plan.h src/spectrum/spectrum_scanner.h
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc/spectrum src/spectrum/spectrum_detect.c src/spectrum/spectrum_plan.c src/spectrum/spectrum_scanner.c tests/test_spectrum_detect.c -o $@

clean:
	rm -rf build
