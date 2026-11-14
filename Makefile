CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic

SPECTRUM_SOURCES = \
	src/spectrum/spectrum_detect.c \
	src/spectrum/spectrum_plan.c \
	src/spectrum/spectrum_scanner.c \
	src/ui/spectrum_view.c \
	src/platform/mono128x64.c
SPECTRUM_HEADERS = \
	src/spectrum/spectrum_detect.h \
	src/spectrum/spectrum_plan.h \
	src/spectrum/spectrum_scanner.h \
	src/ui/spectrum_view.h \
	src/platform/mono128x64.h
RADIO_SOURCES = src/radio/bk4829_bus.c src/radio/bk4829.c src/radio/bk4829_timing.c
RADIO_HEADERS = src/radio/bk4829_bus.h src/radio/bk4829.h src/radio/bk4829_timing.h
INCLUDES = -Isrc/spectrum -Isrc/ui -Isrc/platform -Isrc/radio

.PHONY: test clean

test: build/test_spectrum_detect build/test_bk4829_bus
	./build/test_spectrum_detect
	./build/test_bk4829_bus

build/test_spectrum_detect: $(SPECTRUM_SOURCES) $(SPECTRUM_HEADERS) tests/test_spectrum_detect.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(SPECTRUM_SOURCES) tests/test_spectrum_detect.c -o $@

build/test_bk4829_bus: $(RADIO_SOURCES) $(RADIO_HEADERS) tests/test_bk4829_bus.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(RADIO_SOURCES) tests/test_bk4829_bus.c -o $@

clean:
	rm -rf build
