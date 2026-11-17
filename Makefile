CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic

SPECTRUM_SOURCES = \
	src/spectrum/spectrum_detect.c \
	src/spectrum/spectrum_plan.c \
	src/spectrum/spectrum_scanner.c \
	src/app/spectrum_app.c \
	src/ui/spectrum_view.c \
	src/ui/waterfall.c \
	src/ui/spectrum_key.c \
	src/platform/mono128x64.c \
	src/platform/st7565.c \
	src/platform/py32f071_uvk1_v3.c
SPECTRUM_HEADERS = \
	src/spectrum/spectrum_detect.h \
	src/spectrum/spectrum_plan.h \
	src/spectrum/spectrum_scanner.h \
	src/ui/spectrum_view.h \
	src/ui/waterfall.h \
	src/ui/spectrum_key.h \
	src/platform/mono128x64.h \
	src/app/spectrum_app.h \
	src/platform/st7565.h \
	src/platform/py32f071_uvk1_v3.h
RADIO_SOURCES = src/radio/bk4829_bus.c src/radio/bk4829.c src/radio/bk4829_timing.c src/radio/bk4829_scanner_adapter.c src/radio/bk4829_uvk1_v3_profile.c src/radio/bk4829_uvk1_v3_radio.c
RADIO_HEADERS = src/radio/bk4829_bus.h src/radio/bk4829.h src/radio/bk4829_timing.h src/radio/bk4829_scanner_adapter.h src/radio/bk4829_uvk1_v3_profile.h src/radio/bk4829_uvk1_v3_radio.h
INCLUDES = -Isrc/spectrum -Isrc/ui -Isrc/platform -Isrc/radio -Isrc/app

.PHONY: test clean firmware

test: build/test_waterfall build/test_spectrum_detect build/test_bk4829_bus build/test_st7565 build/test_bk4829_scanner_adapter build/test_spectrum_app build/test_spectrum_view build/test_bk4829_profile build/test_py32_clock build/test_spectrum_key build/test_target_defaults
	./build/test_waterfall
	./build/test_spectrum_detect
	./build/test_bk4829_bus
	./build/test_st7565
	./build/test_bk4829_scanner_adapter
	./build/test_spectrum_app
	./build/test_spectrum_view
	./build/test_py32_clock
	./build/test_spectrum_key
	./build/test_bk4829_profile
	./build/test_target_defaults

build/test_spectrum_detect: $(SPECTRUM_SOURCES) $(SPECTRUM_HEADERS) tests/test_spectrum_detect.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(SPECTRUM_SOURCES) src/radio/bk4829_timing.c tests/test_spectrum_detect.c -o $@

build/test_bk4829_bus: $(RADIO_SOURCES) $(RADIO_HEADERS) tests/test_bk4829_bus.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(RADIO_SOURCES) tests/test_bk4829_bus.c -o $@

build/test_st7565: $(SPECTRUM_SOURCES) $(SPECTRUM_HEADERS) tests/test_st7565.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(SPECTRUM_SOURCES) src/radio/bk4829_timing.c tests/test_st7565.c -o $@

clean:
	rm -rf build

build/test_bk4829_scanner_adapter: $(RADIO_SOURCES) $(RADIO_HEADERS) tests/test_bk4829_scanner_adapter.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(RADIO_SOURCES) tests/test_bk4829_scanner_adapter.c -o $@

build/test_spectrum_app: $(SPECTRUM_SOURCES) $(SPECTRUM_HEADERS) $(RADIO_SOURCES) $(RADIO_HEADERS) tests/test_spectrum_app.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(SPECTRUM_SOURCES) $(RADIO_SOURCES) tests/test_spectrum_app.c -o $@

build/test_bk4829_profile: $(RADIO_SOURCES) $(RADIO_HEADERS) tests/test_bk4829_profile.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(RADIO_SOURCES) tests/test_bk4829_profile.c -o $@

build/test_py32_clock: $(SPECTRUM_SOURCES) $(SPECTRUM_HEADERS) src/radio/bk4829_timing.c tests/test_py32_clock.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(SPECTRUM_SOURCES) src/radio/bk4829_timing.c tests/test_py32_clock.c -o $@

build/test_spectrum_key: src/ui/spectrum_key.c src/ui/spectrum_key.h tests/test_spectrum_key.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) src/ui/spectrum_key.c tests/test_spectrum_key.c -o $@

build/test_target_defaults: src/spectrum/spectrum_plan.c src/spectrum/spectrum_plan.h target/scan_bands.h tests/test_target_defaults.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -Itarget src/spectrum/spectrum_plan.c tests/test_target_defaults.c -o $@

build/test_spectrum_view: $(SPECTRUM_SOURCES) $(SPECTRUM_HEADERS) tests/test_spectrum_view.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(SPECTRUM_SOURCES) tests/test_spectrum_view.c -o $@

build/test_waterfall: src/ui/waterfall.c src/ui/waterfall.h src/spectrum/spectrum_detect.h tests/test_waterfall.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) src/ui/waterfall.c tests/test_waterfall.c -o $@

# Build completo del firmware target PY32F071 / BK4829.
ARM_CC ?= arm-none-eabi-gcc
ARM_OBJCOPY ?= arm-none-eabi-objcopy
ARM_SIZE ?= arm-none-eabi-size
TARGET_CFLAGS = -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -std=c11 -Os \
	-ffreestanding -fno-builtin -fno-common -ffunction-sections -fdata-sections \
	-fstack-usage -fcallgraph-info=su,da -Wstack-usage=768 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra -Werror \
	-Isrc/spectrum -Isrc/ui -Isrc/app -Isrc/platform -Isrc/radio
TARGET_SOURCES = \
	target/startup.c target/runtime.c target/main.c \
	src/spectrum/spectrum_detect.c src/spectrum/spectrum_plan.c \
	src/spectrum/spectrum_scanner.c src/ui/spectrum_view.c \
	src/ui/waterfall.c src/ui/spectrum_key.c src/app/spectrum_app.c \
	src/platform/mono128x64.c src/platform/st7565.c \
	src/platform/py32f071_uvk1_v3.c \
	src/radio/bk4829_bus.c src/radio/bk4829.c \
	src/radio/bk4829_timing.c src/radio/bk4829_scanner_adapter.c \
	src/radio/bk4829_uvk1_v3_profile.c src/radio/bk4829_uvk1_v3_radio.c
TARGET_OBJECTS = $(patsubst %.c,build/target/%.o,$(TARGET_SOURCES))

firmware: build/target/D10S_K1.bin

build/target/%.o: %.c
	mkdir -p $(dir $@)
	$(ARM_CC) $(TARGET_CFLAGS) -c $< -o $@

build/target/D10S_K1.elf: $(TARGET_OBJECTS) target/py32f071xB.ld
	$(ARM_CC) -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -nostdlib \
		-Wl,-T,target/py32f071xB.ld -Wl,--gc-sections -Wl,--build-id=none \
		-Wl,-Map=build/target/D10S_K1.map $(TARGET_OBJECTS) -lgcc -o $@
	$(ARM_SIZE) $@

build/target/D10S_K1.bin: build/target/D10S_K1.elf
	python3 target/check_stack_usage.py build/target
	python3 target/check_callgraph_stack.py build/target --entry Reset_Handler --exception-reserve 40 --budget 1536 --link-map build/target/D10S_K1.map
	$(ARM_OBJCOPY) -O binary $< $@
	python3 target/validate_image.py $@
