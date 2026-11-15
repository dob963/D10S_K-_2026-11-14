# DFU compatibility evidence — UV-K1 bootloader 7.03.01

Audit date: 2026-11-15

## Findings

The diagnostic target image is a **raw Cortex-M firmware image**, not a Quansheng vendor-packed update file. The pinned D10S Run 438 artifact has:

- Length: 14,456 bytes.
- Initial stack pointer: `0x20004000`.
- Reset vector: `0x08002905` (Thumb bit set).
- Linker application origin: `0x08002800`.
- SHA-256: `4fdf289d7eed0b84ec028e62ca3d0627ece83a3f0c2ba42ba9786c6c22cf0a35`.
- DFU transport page count at 256 bytes/page: 57 (last page contains 120 image bytes before transport padding).

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

1. The D10S output should remain a raw binary beginning with its vector table; it must not be XOR-packed or have a version block inserted for this DFU transport.
2. The current 14,456-byte diagnostic image is structurally raw and page-aligned by the transport's 256-byte chunking (the final transport page is padded by the host).
3. The 256-byte page transport and 7.03.01 allowlist are now supported by source review, **not by a D10S hardware flash test**.
4. The D10S linker origin `0x08002800` is a current project configuration. The reviewed DFU sources do not independently prove this origin for every PCB/bootloader revision.
5. The exact board, MCU marking, bootloader readback, application address, and recovery route still require confirmation on the physical radio before a flashable release.

## Source and license record

| Source | Immutable revision | License status | Use in D10S |
|---|---|---|---|
| [knoinok/quansheng_toolkit](https://github.com/knoinok/quansheng_toolkit) | `ea13a5850d8f22372c7ad205db13610c083c4ee8` | Apache-2.0; LICENSE and NOTICE reviewed | Protocol facts only; no code imported |
| [spm81/Multi-UVTools](https://github.com/spm81/Multi-UVTools) | `3979b8097af1703762e472f574219cbc2d35c172` | Not verified | Corroborating behavior notes only; no code imported |
| D10S diagnostic build | `0e68b1ba630a0df03f17b58e397defb23a998d65` | D10S project license not yet selected | Structural image measurements only |

## Release status

**NOT APPROVED FOR FLASHING.** This review closes part of the software-format research gate only. It does not authorize flashing, prove the exact hardware identity, or replace a tested backup and recovery procedure.
