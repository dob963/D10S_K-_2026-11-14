#!/usr/bin/env python3
"""Structural validator for the D10S PY32F071 intermediate application image."""

from __future__ import annotations

import pathlib
import struct
import sys

FLASH_BASE = 0x08002800
FLASH_LIMIT = 0x08020000
RAM_BASE = 0x20000000
RAM_LIMIT = 0x20004000
VECTOR_BYTES = 48 * 4
MAX_IMAGE_BYTES = FLASH_LIMIT - FLASH_BASE


def fail(message: str) -> None:
    raise SystemExit(f"IMAGE CHECK FAILED: {message}")


def main() -> int:
    if len(sys.argv) != 2:
        fail("usage: validate_image.py <raw-image.bin>")

    image_path = pathlib.Path(sys.argv[1])
    data = image_path.read_bytes()
    if len(data) < VECTOR_BYTES:
        fail(f"image too short ({len(data)} bytes; need at least {VECTOR_BYTES})")
    if len(data) > MAX_IMAGE_BYTES:
        fail(f"image too large ({len(data)} bytes; limit {MAX_IMAGE_BYTES})")

    vectors = struct.unpack_from("<48I", data)
    image_end = min(FLASH_BASE + len(data), FLASH_LIMIT)
    initial_sp, reset_vector = vectors[0], vectors[1]

    if initial_sp != RAM_LIMIT:
        fail(f"initial SP 0x{initial_sp:08X} does not equal RAM top 0x{RAM_LIMIT:08X}")
    if initial_sp & 0x7:
        fail("initial SP is not 8-byte aligned")

    reset_address = reset_vector & ~1
    if (reset_vector & 1) == 0:
        fail("Reset_Handler vector does not have the Thumb bit set")
    if not (FLASH_BASE <= reset_address < image_end):
        fail(f"Reset_Handler 0x{reset_vector:08X} does not point into the built image")

    reserved = set(range(4, 11)) | {12, 13, 46}  # IRQ30 is reserved on PY32F071
    for index, vector in enumerate(vectors[1:], start=1):
        if index in reserved:
            if vector != 0:
                fail(f"reserved vector {index} is not zero")
            continue
        address = vector & ~1
        if (vector & 1) == 0:
            fail(f"vector {index} has no Thumb bit")
        if not (FLASH_BASE <= address < image_end):
            fail(f"vector {index} does not point into the built image: 0x{vector:08X}")

    print(
        "STRUCTURAL IMAGE CHECK PASSED: "
        f"{len(data)} bytes, SP=0x{initial_sp:08X}, "
        f"Reset=0x{reset_vector:08X}, flash base=0x{FLASH_BASE:08X}"
    )
    print("This is an intermediate build image, not cleared for radio flashing.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
