#!/usr/bin/env python3
"""Compile the MoonBit application core to portable C for ESP-IDF."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


PACKAGE = "folotoy/strong-password-generator-ai-passport"


def moon_home(moonc: Path) -> Path:
    configured = os.environ.get("MOON_HOME")
    if configured:
        return Path(configured).expanduser().resolve()
    return moonc.resolve().parent.parent


def run(command: list[str]) -> None:
    rendered = " ".join(command)
    print(f"[moonbit-codegen] {rendered}")
    subprocess.run(command, check=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    source_dir = args.source_dir.resolve()
    output = args.output.resolve()
    moonc_name = os.environ.get("MOONC", "moonc")
    moonc_found = shutil.which(moonc_name)
    if not moonc_found:
        print("error: moonc was not found on PATH", file=sys.stderr)
        return 2
    moonc = Path(moonc_found)
    home = moon_home(moonc)
    bundle = home / "lib" / "core" / "_build" / "native" / "release" / "bundle"
    required = [bundle / "prelude" / "prelude.mi", bundle / "abort" / "abort.core", bundle / "core.core"]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        print("error: MoonBit native standard library is incomplete: " + ", ".join(missing), file=sys.stderr)
        return 2

    sources = sorted(
        path for path in source_dir.glob("*.mbt")
        if not path.name.endswith(("_test.mbt", "_wbtest.mbt"))
    )
    if not sources:
        print(f"error: no MoonBit sources found in {source_dir}", file=sys.stderr)
        return 2

    output.parent.mkdir(parents=True, exist_ok=True)
    core = output.with_suffix(".core")
    run([
        str(moonc), "build-package", *map(str, sources),
        "-o", str(core),
        "-pkg", PACKAGE,
        "-pkg-type", "foreign_library",
        "-std-path", str(bundle),
        "-i", f"{bundle / 'prelude' / 'prelude.mi'}:prelude",
        "-pkg-sources", f"{PACKAGE}:{source_dir}",
        "-target", "native",
        "-workspace-path", str(source_dir),
    ])
    run([
        str(moonc), "link-core",
        str(bundle / "abort" / "abort.core"),
        str(bundle / "core.core"),
        str(core),
        "-main", PACKAGE,
        "-o", str(output),
        "-pkg-config-path", str(source_dir / "moon.pkg"),
        "-pkg-sources", f"{PACKAGE}:{source_dir}",
        "-pkg-sources", f"moonbitlang/core:{home / 'lib' / 'core'}",
        "-target", "native",
    ])
    core.unlink(missing_ok=True)
    print(f"[moonbit-codegen] generated {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
