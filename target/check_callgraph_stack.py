#!/usr/bin/env python3
"""Conservative cumulative stack analysis for GCC VCG call-graph reports.

The analysis intentionally fails closed for unresolved calls, dynamic stack
allocation, recursion, and indirect-call proxy nodes reachable from Reset_Handler.
It is a static bound for compiled C code, not a measurement of hardware exception
entry or unmodelled assembly. The PY32F071 exception-frame allowance is added
separately by the caller.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys
from dataclasses import dataclass, field

NODE_RE = re.compile(r"\bnode:\s*\{([^}]*)\}")
EDGE_RE = re.compile(r"\bedge:\s*\{([^}]*)\}")
ATTR_RE = re.compile(r'([A-Za-z_]+):\s*"((?:\\.|[^"])*)"')
STACK_RE = re.compile(r"(\d+) bytes \(([^)]+)\)")
DYNAMIC_RE = re.compile(r"(\d+) dynamic objects")

# Conservative allowance for the ARM EABI unsigned divide/mod helper.
# This is a manual reserve, not a measured compiler-generated frame.
EXTERNAL_STACK_RESERVES = {
    # Conservative whole-helper-chain reserves for linked ARM EABI/libgcc
    # routines. These are not compiler .su measurements; 256 bytes is a
    # deliberately generous bound for each helper and any internal subcalls.
    "__aeabi_uidiv": 256,
    "__aeabi_uidivmod": 256,
    "__aeabi_idiv": 256,
    "__aeabi_idivmod": 256,
    "__aeabi_lmul": 256,
    "__aeabi_uldivmod": 256,
    "__aeabi_ldivmod": 256,
    "__aeabi_llsr": 256,
}

# GCC VCG emits one generic __indirect_call node per translation unit.
# Resolve each call site against the concrete callback targets wired by the
# target integration. The map is deliberately caller-specific to avoid
# creating false cycles by connecting every callback to every other callback.
INDIRECT_CALL_TARGETS = {
    "src/ui/spectrum_view.c:put_pixel.constprop.0": (
        "d10s_mono128x64_pixel",
    ),
    "d10s_spectrum_view_render": (
        "d10s_mono128x64_pixel", "d10s_mono128x64_text",
    ),
    "src/platform/py32f071_uvk1_v3.c:display_delay_ms": (
        "src/platform/py32f071_uvk1_v3.c:systick_delay_ms",
    ),
    "src/platform/py32f071_uvk1_v3.c:radio_delay_ticks": (
        "src/platform/py32f071_uvk1_v3.c:systick_delay_ticks",
    ),
    "src/platform/st7565.c:command": (
        "src/platform/py32f071_uvk1_v3.c:display_data_mode",
        "src/platform/py32f071_uvk1_v3.c:display_write_byte",
    ),
    "src/platform/st7565.c:send_command_frame.constprop.0": (
        "src/platform/py32f071_uvk1_v3.c:display_chip_select",
    ),
    "d10s_st7565_flush_page": (
        "src/platform/py32f071_uvk1_v3.c:display_chip_select",
        "src/platform/py32f071_uvk1_v3.c:display_data_mode",
        "src/platform/py32f071_uvk1_v3.c:display_write_byte",
    ),
    "d10s_st7565_init": (
        "src/platform/py32f071_uvk1_v3.c:display_chip_select",
        "src/platform/py32f071_uvk1_v3.c:display_delay_ms",
    ),
    "src/radio/bk4829_bus.c:recover": (
        "src/platform/py32f071_uvk1_v3.c:radio_set_csn",
        "src/platform/py32f071_uvk1_v3.c:radio_set_clock",
        "src/platform/py32f071_uvk1_v3.c:radio_set_data_output",
        "src/platform/py32f071_uvk1_v3.c:radio_write_data",
    ),
    "src/radio/bk4829_bus.c:begin": (
        "src/platform/py32f071_uvk1_v3.c:radio_set_csn",
        "src/platform/py32f071_uvk1_v3.c:radio_set_clock",
        "src/platform/py32f071_uvk1_v3.c:radio_set_data_output",
        "src/platform/py32f071_uvk1_v3.c:radio_write_data",
        "src/platform/py32f071_uvk1_v3.c:radio_delay_ticks",
    ),
    "src/radio/bk4829_bus.c:write_bits": (
        "src/platform/py32f071_uvk1_v3.c:radio_set_clock",
        "src/platform/py32f071_uvk1_v3.c:radio_write_data",
        "src/platform/py32f071_uvk1_v3.c:radio_delay_ticks",
    ),
    "src/radio/bk4829_bus.c:end": (
        "src/platform/py32f071_uvk1_v3.c:radio_set_csn",
        "src/platform/py32f071_uvk1_v3.c:radio_set_clock",
        "src/platform/py32f071_uvk1_v3.c:radio_set_data_output",
        "src/platform/py32f071_uvk1_v3.c:radio_write_data",
        "src/platform/py32f071_uvk1_v3.c:radio_delay_ticks",
    ),
    "d10s_bk4829_read_register": (
        "src/platform/py32f071_uvk1_v3.c:radio_set_clock",
        "src/platform/py32f071_uvk1_v3.c:radio_set_data_output",
        "src/platform/py32f071_uvk1_v3.c:radio_read_data",
        "src/platform/py32f071_uvk1_v3.c:radio_delay_ticks",
    ),
    "src/radio/bk4829_scanner_adapter.c:fail_closed": (
        "d10s_bk4829_uvk1_v3_stop_rx",
    ),
    "src/radio/bk4829_scanner_adapter.c:tune_hz": (
        "d10s_bk4829_uvk1_v3_resume_rx",
    ),
    "d10s_bk4829_scanner_adapter_init": (
        "d10s_bk4829_uvk1_v3_initialize_rx",
        "d10s_bk4829_uvk1_v3_stop_rx",
    ),
    "d10s_spectrum_scanner_set_attenuation": (
        "src/radio/bk4829_scanner_adapter.c:set_attenuation",
    ),
    "d10s_spectrum_scanner_tick": (
        "src/radio/bk4829_scanner_adapter.c:set_attenuation",
        "src/radio/bk4829_scanner_adapter.c:tune_hz",
        "src/radio/bk4829_scanner_adapter.c:read_rssi",
    ),
}


@dataclass
class Node:
    title: str
    frame: int | None = None
    qualifier: str | None = None
    dynamic_objects: int | None = None
    edges: set[str] = field(default_factory=set)
    source: str = ""


def attributes(body: str) -> dict[str, str]:
    return {key: value for key, value in ATTR_RE.findall(body)}


def parse_reports(root: pathlib.Path) -> dict[str, Node]:
    reports = sorted(root.rglob("*.ci"))
    if not reports:
        raise ValueError("no GCC .ci call-graph reports found")
    graph: dict[str, Node] = {}
    pending_edges: list[tuple[str, str, str]] = []

    for report in reports:
        data = report.read_text(encoding="utf-8")
        for match in NODE_RE.finditer(data):
            attrs = attributes(match.group(1))
            title = attrs.get("title")
            label = attrs.get("label", "")
            if not title:
                continue
            stack = STACK_RE.search(label)
            dynamic = DYNAMIC_RE.search(label)
            frame = int(stack.group(1)) if stack else None
            qualifier = stack.group(2) if stack else None
            dynamic_objects = int(dynamic.group(1)) if dynamic else None

            node = graph.get(title)
            if node is None:
                node = Node(title=title, source=str(report))
                graph[title] = node
            if frame is not None:
                if node.frame is not None and node.frame != frame:
                    raise ValueError(
                        f"conflicting stack frames for {title}: "
                        f"{node.frame} vs {frame}"
                    )
                node.frame = frame
                node.qualifier = qualifier
                node.dynamic_objects = dynamic_objects
                node.source = str(report)

        for match in EDGE_RE.finditer(data):
            attrs = attributes(match.group(1))
            source = attrs.get("sourcename")
            target = attrs.get("targetname")
            if source and target:
                pending_edges.append((source, target, attrs.get("label", "")))

    for name, frame in EXTERNAL_STACK_RESERVES.items():
        node = graph.get(name)
        if node is None:
            graph[name] = Node(
                title=name,
                frame=frame,
                qualifier="static",
                dynamic_objects=0,
                source="manual conservative libgcc reserve",
            )
        elif node.frame is None:
            # GCC can emit a call-target node without a stack label. Apply
            # the explicit reserve to that existing unresolved node too.
            node.frame = frame
            node.qualifier = "static"
            node.dynamic_objects = 0
            node.source = "manual conservative libgcc reserve"

    for source, target, label in pending_edges:
        if source not in graph:
            raise ValueError(f"call-graph edge has unknown caller node: {source}")
        if target == "__indirect_call":
            targets = INDIRECT_CALL_TARGETS.get(source)
            if not targets:
                raise ValueError(
                    f"indirect call site has no reviewed target map: "
                    f"{source} at {label}"
                )
            proxy_name = f"__indirect_call@{source}@{label}"
            proxy = graph.get(proxy_name)
            if proxy is None:
                proxy = Node(
                    title=proxy_name,
                    frame=0,
                    qualifier="static",
                    dynamic_objects=0,
                    source="reviewed target integration map",
                )
                graph[proxy_name] = proxy
            for callback in targets:
                if callback not in graph:
                    raise ValueError(
                        f"mapped indirect callback is absent from call graph: "
                        f"{callback} (caller {source})"
                    )
                proxy.edges.add(callback)
            graph[source].edges.add(proxy_name)
        else:
            graph[source].edges.add(target)
    return graph


def worst_path(graph: dict[str, Node], root: str) -> tuple[int, list[str]]:
    if root not in graph:
        raise ValueError(f"entry point {root!r} is missing from call graph")
    visiting: set[str] = set()
    memo: dict[str, tuple[int, list[str]]] = {}

    def visit(name: str) -> tuple[int, list[str]]:
        if name in memo:
            return memo[name]
        if name in visiting:
            raise ValueError(f"recursive call cycle reaches {name}")
        node = graph.get(name)
        if node is None:
            raise ValueError(f"unresolved external/indirect call target: {name}")
        if node.frame is None:
            raise ValueError(f"no stack annotation for reachable call target: {name}")
        if node.qualifier not in ("static", "dynamic,bounded"):
            raise ValueError(
                f"unbounded/dynamic stack frame at {name}: {node.qualifier}"
            )
        if node.dynamic_objects is None:
            raise ValueError(f"missing dynamic-object annotation for {name}")
        if node.dynamic_objects:
            raise ValueError(
                f"dynamic stack object(s) at {name}: {node.dynamic_objects}"
            )

        visiting.add(name)
        best_child = (0, [])
        for child in sorted(node.edges):
            child_result = visit(child)
            if child_result[0] > best_child[0]:
                best_child = child_result
        visiting.remove(name)
        result = (node.frame + best_child[0], [name] + best_child[1])
        memo[name] = result
        return result

    return visit(root)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("build_dir", type=pathlib.Path)
    parser.add_argument("--entry", default="Reset_Handler")
    parser.add_argument("--exception-reserve", type=int, default=40)
    parser.add_argument("--budget", type=int, default=1536)
    args = parser.parse_args()

    try:
        graph = parse_reports(args.build_dir)
        c_bytes, chain = worst_path(graph, args.entry)
        total = c_bytes + args.exception_reserve
        print("CUMULATIVE STACK PATH (worst direct-call chain):")
        for index, title in enumerate(chain):
            node = graph[title]
            print(f"  {index + 1:02d}. {title}: {node.frame} bytes")
        print(
            f"STATIC C CALL-CHAIN: {c_bytes} bytes; "
            f"exception reserve: {args.exception_reserve} bytes; "
            f"budget: {args.budget} bytes"
        )
        print(f"TOTAL CONSERVATIVE ESTIMATE: {total} bytes")
        if total > args.budget:
            raise ValueError(
                f"stack budget exceeded: {total} > {args.budget} bytes"
            )
        print(
            f"STACK ANALYSIS PASSED: {total}/{args.budget} bytes; "
            f"{args.budget - total} bytes remain in analyzed budget"
        )
        return 0
    except (OSError, ValueError) as error:
        print(f"STACK ANALYSIS FAILED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
