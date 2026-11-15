# D10S UV-K1 bootloader / DFU compatibility evidence

Audit date: 2026-11-15

## Current D10S release candidate

- Workflow: [run 43749934363](https://github.com/dob963/D10S_K-_2026-11-14/actions/runs/43749934363).
- Commit: `31e792e4a2fe5ab201e10bb478872b76b7af9b5f`.
- Conclusion: success; compile, tests, Cortex-M0+ cross-compile, PY32F071 link/image validation, stack diagnostics, Makefile tests and artifact upload all completed.
- Artifact: `D10S-K1-firmware-release-candidate`, ID `14175781912`.
- ZIP size: 37,601 bytes; SHA-256: `9d2e408f74f2b1340366e084b5f35b02c09b512b844f254143b76191316a0845`.
- Extracted raw image: `D10S_K1.bin`, 15,196 bytes; SHA-256: `41d25c6281c90848c01f3c7542961f56ade79510d539330efd635cb5b96727d9`.
- Vector values: initial SP `0x20004000`; Reset vector `0x08002905` (Thumb).
- Build metadata: `RELEASE_CANDIDATE_HARDWARE_VALIDATION_REQUIRED`.

The image is a raw Cortex-M release candidate, not a Quansheng vendor-packed stock update. The label change does not itself prove that the physical radio will accept or boot it.

## Findings from public DFU implementations

The public `knoinok/quansheng_toolkit` project, revision `ea13a5850d8f22372c7ad205db13610c083c4ee8`, is Apache-2.0 licensed; its LICENSE and NOTICE were reviewed. Its reference implementation documents:
- UV-K1 bootloader `7.03.01` as a known UV-K1 bootloader and includes it in the K1 flash allowlist;
- serial DFU transport at 38,400 baud;
- 256-byte firmware pages, page index/count and per-page acknowledgement;
- raw ARM firmware sent unchanged, with a separate decoding path for vendor-packed stock firmware.

A separate published implementation, `spm81/Multi-UVTools` at revision `3979b8097af1703762e472f574219cbc2d35c172`, documents the same 256-byte indexed page transfer. Its repository license was not verified; no source code from it was copied or imported.

## Model distinction

The toolkit's model map identifies:
- `7.03.01` as **UV-K1**;
- `7.00.07` as **UV-K5 V3 / UV-K1(8) V3 Mini Kong**.

The reported bootloader supports the standard UV-K1 target classification. It does not prove the exact PCB revision or MCU marking in the user's radio.

## D10S implications and remaining gates

1. Keep the D10S output as a raw Cortex-M image beginning with its vector table. Do not apply stock-firmware XOR packing or insert a vendor version block.
2. The 256-byte transport and 7.03.01 allowlist are supported by source review, not by a D10S hardware flash test.
3. The linker origin `0x08002800` is the current project configuration. The reviewed DFU sources do not independently prove this origin for every PCB/bootloader revision.
4. The exact board, MCU marking, bootloader readback, application address and recovery route still require confirmation on the physical radio.
5. A successful CI build and structural vector check do not prove RF, LCD, keypad, bootloader hand-off or recovery behavior.

## Source and license record

| Source | Immutable revision | License status | Use in D10S |
|---|---|---|---|
| [knoinok/quansheng_toolkit](https://github.com/knoinok/quansheng_toolkit) | `ea13a5850d8f22372c7ad205db13610c083c4ee8` | Apache-2.0; LICENSE and NOTICE reviewed | Protocol facts only; no code imported |
| [spm81/Multi-UVTools](https://github.com/spm81/Multi-UVTools) | `3979b8097af1703762e472f574219cbc2d35c172` | Not verified | Corroborating protocol notes only; no code imported |
| D10S release candidate | `31e792e4a2fe5ab201e10bb478872b76b7af9b5f` | Project license not yet selected | Original project build; hardware validation pending |

## Release status

**HARDWARE VALIDATION REQUIRED — NOT YET APPROVED FOR FLASHING.**

This is a release candidate with a successful software build. It is not yet a hardware-validated firmware release. Confirm the exact radio, preserve its stock firmware and prove a recovery path before any controlled installation test.
