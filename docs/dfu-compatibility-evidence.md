# DFU compatibility evidence — UV-K1 bootloader 7.03.01

Audit date: 2026-11-15

## Findings

The D10S target workflow produces a raw Cortex-M firmware image, not a Quansheng vendor-packed update file. The latest verified CI artifact is:

- Workflow run: [43699850862](https://github.com/dob963/D10S_K-_2026-11-14/actions/runs/43699850862).
- Commit: `9185380051142a69e35a92dbce63a8fece95198c`.
- Artifact: `D10S-K1-diagnostic-build-not-for-flashing`.
- Artifact ID: `14155852797`.
- ZIP size: 36,448 bytes.
- ZIP SHA-256: `25f64131ff7c01fa3b4030118948dcc2d2acdb7041af0d6114f8338a207b7f5e`.

The workflow confirms successful warning-as-error compilation, CMake tests, Cortex-M0+ cross-compilation, full PY32F071 application link, image-layout validation, Makefile build/tests, and diagnostic artifact upload.

**Extracted-image inspection for run 43699850862:** the artifact ZIP was downloaded and independently verified. It contains `D10S_K1.intermediate.bin` (14,692 bytes; SHA-256 `0c4a1837c442b99ca5f5dac330eadd935cc05d87588203ab7f5b58b4e223951a`), `D10S_K1.elf` (44,864 bytes; SHA-256 `15b03ad3d5624d12690f16f393c37b87f6165e67078df36916eb59f422a2ddf4`), a 51,493-byte map and build metadata. The ELF identifies ARM EABI5 output. The extracted raw image has initial SP `0x20004000`, reset vector `0x08002905` (Thumb), vector table at `0x08002800`; linked `.text` starts at `0x08002900`, and `.bss` ends at `0x200015c0`. The ZIP digest was independently recomputed and matches the GitHub Actions artifact digest. These are structural build observations only; they do not prove the application origin or bootloader compatibility on the physical radio.

The public `knoinok/quansheng_toolkit` project, revision `ea13a5850d8f22372c7ad205db13610c083c4ee8`, is Apache-2.0 licensed. Its DFU reference implementation documents:
- UV-K1 bootloader `7.03.01` as a known UV-K1 bootloader and includes it in the K1 flash allowlist.
- DFU serial transport at 38,400 baud.
- 256-byte firmware pages, page index/count, and a per-page acknowledgement.
- Raw ARM firmware is sent unchanged. Vendor-packed firmware is decoded separately (XOR transformation, optional trailing CRC removal, and removal of the 16-byte version block at offset `0x2000`).

A separate published implementation, `spm81/Multi-UVTools` at revision `3979b8097af1703762e472f574219cbc2d35c172`, documents the same 256-byte indexed page transfer. Its repository license was not verified during this audit. **No source code from that repository was copied or imported**; it is recorded only as corroborating protocol documentation.

## Important model distinction

The toolkit's model map identifies:
- `7.03.01` as **UV-K1**.
- `7.00.07` as **UV-K5 V3 / UV-K1(8) v3 Mini Kong**.

Therefore, the reported bootloader `7.03.01` supports the standard UV-K1 target classification; it does **not** prove that a UV-K1(8) v3 Mini Kong is the user's exact hardware. The printed model and MCU marking must be confirmed before release. Do not assume that the two model variants are interchangeable.

## D10S implications

1. The D10S output should remain a raw Cortex-M image beginning with its vector table; it must not be XOR-packed or have a vendor version block inserted for this DFU transport.
2. The 256-byte page transport and 7.03.01 allowlist are supported by source review, **not by a D10S hardware flash test**.
3. The D10S linker origin `0x08002800` is a current project configuration. The reviewed DFU sources do not independently prove this origin for every PCB/bootloader revision.
4. The exact board, MCU marking, bootloader readback, application address, and recovery route still require confirmation on the physical radio before a flashable release.
5. The latest CI artifact is diagnostic-only. A successful build and image-layout check do not authorize flashing.

## Source and license record

| Source | Immutable revision | License status | Use in D10S |
|---|---|---|---|
| [knoinok/quansheng_toolkit](https://github.com/knoinok/quansheng_toolkit) | `ea13a5850d8f22372c7ad205db13610c083c4ee8` | Apache-2.0; LICENSE and NOTICE reviewed | Protocol facts only; no code imported |
| [spm81/Multi-UVTools](https://github.com/spm81/Multi-UVTools) | `3979b8097af1703762e472f574219cbc2d35c172` | Not verified | Corroborating behavior notes only; no code imported |
| D10S diagnostic build | `9185380051142a69e35a92dbce63a8fece95198c` | D10S project license not yet selected | CI build; raw BIN extracted and hashed; diagnostic only, not for flashing |

## Release status

**NOT APPROVED FOR FLASHING.** This review closes part of the software-format research gate only. It does not authorize flashing, prove the exact hardware identity, or replace a tested backup and recovery procedure.
