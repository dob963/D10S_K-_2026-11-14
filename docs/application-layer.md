# Portable Spectrum application layer

`src/app/spectrum_app.c` coordinates the portable modules. It owns the scanner, copies the radio callbacks, owns the 1024-byte framebuffer and receives an initialized generic ST7565 controller.

On each application tick it:
1. advances one non-blocking tune/settle/read scanner step;
2. waits until a whole sweep has been processed and published;
3. fetches the snapshot together with its band and ATT tags;
4. builds the view model from the just-completed detector result;
5. renders the 128x64 graph into the monochrome framebuffer;
6. transfers all eight framebuffer pages through the ST7565 controller.

The application exposes wrapper functions for cursor, font weight, Delay, Turbo, ATT and manual band selection. A key handler maps the basic UV-K1 keypad to cursor movement, cursor toggle/center, font weight and deferred band selection at a sweep boundary. ATT/Delay/Turbo key menus are not enabled until their hardware settings are validated.

## Test coverage

`tests/test_spectrum_app.c` runs three complete 5-bin scans against a simulated radio, confirms a test peak, renders each finished sweep and verifies three complete eight-page transfers (3 × 8 × 131 bytes) through a simulated ST7565 port. It also verifies that initialization is rejected when the display controller has not been initialized.

## Hardware boundary

The app does not initialize the radio or LCD. It must receive:
- a radio adapter whose RX initialization, tune and RSSI read callbacks are valid for the exact board;
- an ST7565 controller initialized with the real panel's settings and physical GPIO/SPI callbacks.

The `d10s_bk4829_uvk1_v3_radio_init()` wrapper now binds the source-derived UV-K1 V3 receive profile and scanner adapter directly into the radio-ops object consumed by the app. It still requires a separately verified physical GPIO bus implementation, and ATT remains unavailable. The portable app is therefore integrated across its software interfaces, not yet a complete target firmware.