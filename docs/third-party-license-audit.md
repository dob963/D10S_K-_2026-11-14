# Third-party source and license audit

Audit date: 2026-11-14

## Policy

D10S is developed as an independent, receive-only Spectrum Analyzer. Public availability of a repository does not grant permission to copy, modify, or redistribute its code. Before importing any third-party implementation, record the exact repository, immutable revision, files, license, copyright notices, modifications, and required accompanying notices.

Do not copy code from a project whose license is missing, unclear, incompatible with the intended distribution, or not verified at the exact revision. In those cases, use only independently verifiable functional observations and write a clean implementation without copying source text or distinctive implementation structure. A clean-room rewrite is not a way to evade patent, trademark, contractual, or other legal restrictions; obtain legal review when those issues arise.

## Reviewed references

| Project | Purpose of review | License status | D10S code reuse |
|---|---|---|---|
| [armel/uv-k1-k5v3-firmware-custom](https://github.com/armel/uv-k1-k5v3-firmware-custom) | PY32F071 platform, BK4829, LCD/keypad and build conventions | Apache-2.0 verified at pinned revision `3261ca7883d5b0b60c0905d8766612dee7b4191d`; source header, LICENSE and NOTICE reviewed | Modified BK4829 reference profile in `src/radio/bk4829_uvk1_v3_profile.c/.h`, using `App/driver/bk4829.c`, `App/radio.c` and `App/driver/bk4819-regs.h`; notices and license copy included |
| [Public UV-K1 V3 firmware reference](https://github.com/autumn1rain/karina-v3-k1) | Spectrum workflow and visual/interaction requirements | Apache-2.0 as recorded during repository review; exact revision must be re-checked before any future code reuse | No source imported; published behavior notes only |
| [Puya PY32F071 SDK files in the pinned platform reference](https://github.com/armel/uv-k1-k5v3-firmware-custom/tree/3261ca7883d5b0b60c0905d8766612dee7b4191d/Drivers) | Device register definitions, LL GPIO/SPI headers, startup and linker reference | BSD-3-Clause notices by Puya Semiconductor and STMicroelectronics were checked in the relevant file headers | Reference only; no SDK, startup, linker or HAL source imported into D10S |
| [Dim0s/quansheng-UV-K5v3-K1-firmware-pack](https://github.com/Dim0s/quansheng-UV-K5v3-K1-firmware-pack) | Maintainer notes about Spectrum behavior and known defects | License not verified in this audit | Notes only; no code imported |
| KA52M K1/V3 firmware | BK4829 driver and Spectrum implementation research lead | Exact repository/revision and license not yet verified in this audit | No code imported |
| Fagci K1 scanner | Scan/acquisition architecture research lead | Exact repository/revision and license not yet verified in this audit | No code imported |

This table records the state of the review, not a blanket legal opinion about the listed projects. Re-check repository license files, file-level headers, contributor notices, and the exact commit before any future reuse.


| `armel/uv-k1-k5v3-firmware-custom` — Fox Hunt gain ladder | BK4829 REG_13 ATT/BYP receive-gain presets | Apache-2.0 verified at pinned revision `3261ca7883d5b0b60c0905d8766612dee7b4191d`; `App/app/foxhunt.c` header and repository LICENSE reviewed | Modified six-step gain preset adaptation in `src/radio/bk4829.c`; copyright retained in file header and `THIRD_PARTY_NOTICES.md` |

## Current D10S repository licensing

At this audit, the repository has no project-level `LICENSE` file. Therefore no open-source license has yet been granted for original D10S code. The Apache-2.0 file under `LICENSES/` applies to the identified source-derived BK4829 profile only; it does not license the whole D10S repository. Public visibility on GitHub is not a license to reuse it. Do not add a license or advertise D10S as open source until the project owner has selected and approved the license.

## Required import checklist

Before a third-party code import:

1. Pin the source to an immutable commit SHA and record the exact files.
2. Read the repository license and any file-level copyright/license headers.
3. Check compatibility with the D10S distribution model and all dependencies.
4. Preserve copyright, attribution, patent, trademark, and disclaimer notices required by that license.
5. Include the full license text and any required NOTICE/attribution material in the distributed source and binary package.
6. Mark modified files and describe material changes where the license requires it.
7. Record the imported code, author, source URL, revision, license, and local destination in this audit.
8. Run a license/dependency scan before each release and review all newly added files.
9. If the license or provenance is unclear, do not import the code; implement independently from documented behavior or seek permission.

## Release rule

No D10S release may be represented as legally cleared for redistribution until the project license is selected, all third-party components and dependencies are inventoried, and the required notices are included in the source and binary release package.
