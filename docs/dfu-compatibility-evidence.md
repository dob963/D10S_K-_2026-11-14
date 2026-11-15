# DFU compatibility evidence — UV-K1 bootloader 7.03.01

Audit date: 2026-11-15

## Findings

The D10S target workflow produces a raw Cortex-M firmware image, not a Quansheng vendor-packed update file. The latest verified CI artifact is:

- Workflow run: [43699594411](https://github.com/dob963/D10S_K-_2026-11-14/actions/runs/43699594411).
- Commit: `c8be5455f01111d104a9082c55ffcbb02a358184`.
- Artifact: `D10S-K1-diagnostic-build-not-for-flashing`.
- Artifact ID: `14156051824`.
- ZIP size: 36,449 bytes.
- ZIP SHA-256: `1327032170f56525e8fd348a68ef0736b2bbb56f8073ae2d65b5dddef2e7c27b`.

The workflow confirms successful warning-as-error compilation, CMake tests, Cortex-M0+ cross-compilation, full PY32F071 application link, image-layout validation, Makefile build/tests, and diagnostic artifact upload.

**Important evidence correction:** earlier revisions of this file recorded image length, vector values and image SHA-256 from a different diagnostic build. Those measurements are not carried forward as measurements of run 43699594411. The latest artifact ZIP has been identified and checksummed, but its contained BIN has not been separately extracted and re-hashed in this audit. Therefore no raw-image length, vector values, or raw-image digest are asserted for the latest run here.

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
| D10S diagnostic build | `c8be5455f01111d104a9082c55ffcbb02a358184` | D10S project license not yet selected | CI build and artifact metadata only; latest raw BIN not independently hashed |

## Release status

**NOT APPROVED FOR FLASHING.** This review closes part of the software-format research gate only. It does not authorize flashing, prove the exact hardware identity, or replace a tested backup and recovery procedure.
