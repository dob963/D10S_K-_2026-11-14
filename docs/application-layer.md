# Portable Spectrum application layer

`src/app/spectrum_app.c` coordinates the portable modules. It owns the scanner, copies the radio callbacks, owns the 1024-byte framebuffer and receives an initialized generic ST7565 controller.

On each application tick it:
1. advances one non-blocking tune/settle/read scanner step;
2. waits until a whole sweep has been processed and published;
3. fetches the snapshot together with its band and ATT tags;
4. builds the view model from the just-completed detector result;
5. renders the 128x64 graph into the monochrome framebuffer;
6. transfers all eight framebuffer pages through the ST7565 controller.

The application exposes wrapper functions for cursor, font weight, Delay, Turbo, ATT and manual band selection. The current key handler maps cursor movement (UP/DOWN and side keys), cursor toggle/center (MENU/0), cursor clear (EXIT), font weight (F), peak-hold marker visibility (STAR), deferred band selection (1–2) at a sweep boundary, scan freeze/resume (9), and noise-margin adjustment (6 decreases by 2 raw RSSI units, 7 increases by 2, 8 restores the default margin of 12; range 2–64). The current detector threshold is always noise floor plus this adjustable margin. Key 3 toggles adaptive Turbo, key 4 cycles Delay through 8/12/16/24 ms, and key 5 cycles six REG_13 gain presets. These controls are software-integrated; the physical keypad mapping, RF response, settling times and gain behavior remain unvalidated on the actual board. The six gain presets are not calibrated attenuation measurements.

Freeze republishes the last complete snapshot with a hold state and stops further tune/RSSI operations. On resume, the incomplete sweep is discarded and scanning restarts at bin zero of the selected band, while the last published graph remains visible until the next full sweep.

## Test coverage

`tests/test_spectrum_app.c` runs complete simulated scans, confirms a test peak, renders each finished sweep and verifies complete eight-page transfers through a simulated ST7565 port. It also verifies that initialization is rejected when the display controller has not been initialized. Separate scanner and radio tests cover Delay/Turbo scheduling and the software REG_13 write path; they do not validate RF gain or physical timing.

## Hardware boundary

The app does not initialize the radio or LCD. It must receive:
- a radio adapter whose RX initialization, tune and RSSI read callbacks are valid for the exact board;
- an ST7565 controller initialized with the real panel's settings and physical GPIO/SPI callbacks.

The `d10s_bk4829_uvk1_v3_radio_init()` wrapper binds the source-derived UV-K1 V3 receive profile and scanner adapter directly into the radio-ops object consumed by the app. It still requires a separately verified physical GPIO bus implementation. ATT presets are wired through REG_13 but have not been electrically measured or calibrated on the user's board. The portable app is therefore integrated across its software interfaces, not yet a complete hardware-validated firmware.
