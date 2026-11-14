#!/usr/bin/env python3
"""Report and bound individual C stack frames in the PY32F071 target build."""

from __future__ import annotations

import pathlib
import sys

MAX_FRAME_BYTES = 768


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_stack_usage.py <target-build-dir>")

    root = pathlib.Path(sys.argv[1])
    reports = sorted(root.rglob("*.su"))
    if not reports:
        raise SystemExit("STACK CHECK FAILED: no GCC .su reports found")

    maximum = (0, "", "")
    entries = 0
    for report in reports:
        for line_number, line in enumerate(
            report.read_text(encoding="utf-8").splitlines(), start=1
        ):
            fields = line.rsplit("\t", 2)
            if len(fields) != 3:
                continue
            try:
                frame_bytes = int(fields[1])
            except ValueError:
                continue
            function = fields[0]
            kind = fields[2]
            entries += 1
            if frame_bytes > maximum[0]:
                maximum = (frame_bytes, function, kind)
            if frame_bytes > MAX_FRAME_BYTES:
                raise SystemExit(
                    f"STACK CHECK FAILED: {function} uses {frame_bytes} bytes "
                    f"({kind}); limit is {MAX_FRAME_BYTES}"
                )

    if entries == 0:
        raise SystemExit("STACK CHECK FAILED: no stack entries parsed")

    print(
        f"STACK CHECK PASSED: {entries} function frames, maximum "
        f"{maximum[0]} bytes in {maximum[1]} ({maximum[2]}); "
        f"per-function limit {MAX_FRAME_BYTES} bytes"
    )
    print("This bounds individual frames, not the complete nested call chain.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
