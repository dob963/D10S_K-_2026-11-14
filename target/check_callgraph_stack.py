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
    "__aeabi_uidivmod": 256,
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
    pending_edges: list[tuple[str, str]] = []

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
                pending_edges.append((source, target))

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

    for source, target in pending_edges:
        if source in graph:
            graph[source].edges.add(target)
        else:
            raise ValueError(f"call-graph edge has unknown caller node: {source}")
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
