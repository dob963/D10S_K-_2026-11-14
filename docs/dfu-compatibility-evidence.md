# D10S UV-K1 bootloader / DFU compatibility evidence

Audit date: 2026-11-15

## Current D10S release candidate

- Workflow: [run 43750817987](https://github.com/dob963/D10S_K-_2026-11-14/actions/runs/43750817987), on source commit `9eb0f341d72f890227bd97103878f9396efd38a5`.
- Firmware source commit: `31e792e4a2fe5ab201e10bb478872b76b7af9b5f`; validator correction included in CI at `9eb0f341d72f890227bd97103878f9396efd38a5`.
- Conclusion: success; compile, tests, Cortex-M0+ cross-compile, PY32F071 link/image validation, stack diagnostics, Makefile tests and artifact upload all completed.
- Latest artifact: `D10S-K1-firmware-release-candidate`, ID `14175588776` (run 43750817987).
- GitHub artifact ZIP size: 37,600 bytes; GitHub SHA-256: `7cf26e104dac15cab1ce5dd0497ec01460090924af5c5167f02ead43ec49893e`.
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


## UV Air Tool 4.7.4 rejection observed on the user's PC

On 2026-11-15, the user supplied a screenshot of UV Air Tool 4.7.4 in `UV-K1 | FLASH MODE`. With `D10S_K1.bin` selected and the `KA-52M FIRMWARE` profile shown, the application reported: `ERROR, FIRMWARE FOR: UV-K5 V1` and `Selected firmware is for: UV-K5 V1. It is not compatible with UV-K1.`

This is a real tool-compatibility failure. The D10S candidate must not be flashed through this application in its current state. The message does not, by itself, prove that the MCU target is actually DP32G030; it proves only that UV Air Tool classified the selected file as UV-K5 V1.

The OURO product page distinguishes KA-52M (UV-K1 and K5(8)/K6/R5 V3, bootloader 7.x) from SU-75 (UV-K5 V1, bootloader 2/3/4/5). A public OURO flasher repository also contains a Python serial flasher, but that repository has no declared license in the reviewed tree. Its code was inspected only to understand the transport and was not copied or reused. It is not established that this separate script implements the same file-classification logic as UV Air Tool 4.7.4.

A separate Apache-2.0 reference implementation (pinned above) accepts raw Cortex-M binaries unchanged when the first word is a valid RAM stack pointer. This supports raw-image transport as one documented DFU path, but it does not override the observed rejection by UV Air Tool. No alternate flasher is approved for this D10S image until the image layout and the exact radio application origin are independently verified.

### Required correction and release gate

1. Inspect the UV Air Tool 4.7.4 file-classification rules or obtain a known-good UV-K1 raw image and compare the format without modifying or repacking D10S blindly.
2. Confirm the standard UV-K1 application base address for bootloader 7.03.01 from a known-good UV-K1 image or an authoritative implementation; the current D10S linker origin `0x08002800` remains unproven for this physical unit.
3. Add an offline validator that checks the D10S image against the verified UV-K1 image structure and target-identification convention.
4. Rebuild and require UV Air Tool to identify the image as UV-K1 before any controlled flash test.

**Current status: REJECTED BY UV AIR TOOL 4.7.4 — DO NOT FLASH.**


## Offline inspection of the rejected D10S candidate

The retained candidate at commit `31e792e4a2fe5ab201e10bb478872b76b7af9b5f` was inspected locally without modifying it:

- File length: 15,196 bytes.
- SHA-256: `41d25c6281c90848c01f3c7542961f56ade79510d539330efd635cb5b96727d9`.
- First vector word (initial MSP): `0x20004000`.
- Second vector word (Reset_Handler): `0x08002905`.
- The image is a raw Cortex-M binary; it does not contain a readable model/version string in the initial vector table.

The public Apache-2.0 `armel/uv-k1-k5v3-firmware-custom` serial-tool codec documents the Quansheng packed updater format separately: XOR obfuscation, a 16-byte version field inserted at offset `0x2000`, and a trailing CRC16/XMODEM. Its raw-image detector checks the RAM stack-pointer and flash reset-vector address ranges. The D10S candidate satisfies those two vector-range checks. This is evidence that the image is structurally raw, not proof that it matches every UV Air Tool 4.7.4 target-identification rule or the correct physical UV-K1 application origin.

The upstream F4HWN release notes identify the newer UV-K1/UV-K5 V3 platform as PY32F071, distinct from the older UV-K5 V1 DP32G030 platform. D10S is built for PY32F071. Therefore, the screenshot's UV-K5 V1 classification conflicts with the D10S build target; it must be resolved by inspecting the exact UV Air Tool 4.7.4 parser or comparing against a known-good UV-K1 7.03.01 image. Do not add a guessed version block or alter the raw image merely to change the displayed label.

### Additional gate

Before any flash attempt, compare the candidate against a known-good UV-K1 7.03.01 firmware image and verify the exact application origin, vector table, version/target metadata expected by the specific flasher, and image length limits. A successful raw-vector check alone is insufficient.
