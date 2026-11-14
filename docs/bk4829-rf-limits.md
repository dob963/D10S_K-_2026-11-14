# BK4829 RF operating windows

The D10S hardware adapter enforces the BK4829 datasheet operating windows:

- 18–580 MHz
- 760–1160 MHz

The 580–760 MHz gap is not a supported BK4829 operating range. Frequencies above 1160 MHz, including 1300 MHz, are also rejected. These are IC limits, not a promise that every frequency inside the windows is usable with the UV-K1 matching network, filters, antenna or regional rules.

The portable band planner remains hardware-independent. The BK4829 radio layer rejects an unsupported frequency before it writes the synthesizer registers. A band list intended for the radio must therefore be split into valid windows and checked against the specific board's RF front end.

Frequency programming uses the documented 10 Hz word unit and writes REG_38 (low word) followed by REG_39 (high word). This primitive does not perform the receiver mode transition, wait for synthesizer lock, or prove RF reception. A failed second register write is a partial programming failure; the future target adapter must fail closed and request a verified standby/reinitialization path.

Evidence: Beken BK4829 datasheet DS-BK4829-E01 V1.0, Worldwide band and operating-frequency specifications. See the [datasheet extract and frequency specification](https://manuals.plus/m/b03fb6dba016158ab9f4b5b3d1aeb1d9ea178efc44cc11c2c08398621553cdc9.pdf).
