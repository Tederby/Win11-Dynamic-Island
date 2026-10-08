#!/usr/bin/env python3
"""
Windhawk Mod Bundler
Combines modular C++ sources, metadata, and Windhawk headers into a monolithic .wh.cpp file.
"""

import os
import sys
import re
import time
import argparse
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT_DIR / "src"
DEFAULT_OUTPUT = ROOT_DIR / "win11-dynamic-island.wh.cpp"

SYSTEM_INCLUDE_REGEX = re.compile(r'^\s*#include\s*<([^>]+)>\s*$')
LOCAL_INCLUDE_REGEX = re.compile(r'^\s*#include\s*"([^"]+)"\s*$')
PRAGMA_ONCE_REGEX = re.compile(r'^\s*#pragma\s+once\s*$')

def read_file_content(path: Path) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()

class Bundler:
    def __init__(self, src_dir: Path, output_file: Path):
        self.src_dir = src_dir
        self.output_file = output_file
        self.included_files = set()
        self.system_includes = []
        self.system_includes_set = set()

    def process_file(self, file_path: Path) -> list:
        canonical_path = file_path.resolve()
        if canonical_path in self.included_files:
            return []
        self.included_files.add(canonical_path)

        rel_path = file_path.relative_to(ROOT_DIR) if file_path.is_relative_to(ROOT_DIR) else file_path.name
        output_lines = [f"\n// ============================================================================",
                        f"// [Module] {rel_path}",
                        f"// ============================================================================\n"]

        with open(canonical_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        for line in lines:
            line_str = line.rstrip("\r\n")

            # Pragma once is handled by file tracker
            if PRAGMA_ONCE_REGEX.match(line_str):
                continue

            # System includes <...>
            sys_match = SYSTEM_INCLUDE_REGEX.match(line_str)
            if sys_match:
                header = sys_match.group(1).strip()
                if header not in self.system_includes_set:
                    self.system_includes_set.add(header)
                    self.system_includes.append(header)
                continue

            # Local includes "..."
            local_match = LOCAL_INCLUDE_REGEX.match(line_str)
            if local_match:
                target_rel = local_match.group(1)
                target_path = (file_path.parent / target_rel).resolve()
                if not target_path.exists():
                    # Fallback to search from src_dir
                    target_path = (self.src_dir / target_rel).resolve()

                if not target_path.exists():
                    raise FileNotFoundError(f"Cannot resolve include \"{target_rel}\" in {file_path}")

                sub_lines = self.process_file(target_path)
                output_lines.extend(sub_lines)
                continue

            output_lines.append(line_str)

        return output_lines

    def build(self) -> bool:
        self.included_files.clear()
        self.system_includes.clear()
        self.system_includes_set.clear()

        metadata_header = self.src_dir / "metadata" / "mod_header.h"
        metadata_readme = self.src_dir / "metadata" / "mod_readme.h"
        metadata_settings = self.src_dir / "metadata" / "mod_settings.h"
        main_entry = self.src_dir / "main.cpp"

        if not main_entry.exists():
            print(f"[Error] Entry point not found: {main_entry}", file=sys.stderr)
            return False

        print(f"[*] Bundling from: {main_entry}")
        body_lines = self.process_file(main_entry)

        # Assemble monolithic output
        result = []

        # 1. Metadata blocks
        if metadata_header.exists():
            result.append(read_file_content(metadata_header).strip())
            result.append("\n")

        if metadata_readme.exists():
            result.append(read_file_content(metadata_readme).strip())
            result.append("\n")

        if metadata_settings.exists():
            result.append(read_file_content(metadata_settings).strip())
            result.append("\n")

        # 2. System includes banner
        result.append("// ============================================================================")
        result.append("// System Includes")
        result.append("// ============================================================================")
        for sys_hdr in self.system_includes:
            result.append(f"#include <{sys_hdr}>")
        result.append("\n")

        # 3. Code content
        result.append("\n".join(body_lines))

        # Write output
        final_code = "\n".join(result) + "\n"
        self.output_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.output_file, "w", encoding="utf-8") as f:
            f.write(final_code)

        print(f"[+] Successfully bundled {len(self.included_files)} modules -> {self.output_file}")
        return True

def watch_mode(bundler: Bundler, src_dir: Path):
    print(f"[*] Watching {src_dir} for changes... (Press Ctrl+C to exit)")
    bundler.build()
    last_mtimes = {}

    def get_all_mtimes():
        mtimes = {}
        for root, _, files in os.walk(src_dir):
            for file in files:
                if file.endswith((".h", ".hpp", ".c", ".cpp", ".yaml", ".md")):
                    p = Path(root) / file
                    try:
                        mtimes[p] = p.stat().st_mtime
                    except OSError:
                        pass
        return mtimes

    last_mtimes = get_all_mtimes()

    try:
        while True:
            time.sleep(1)
            current_mtimes = get_all_mtimes()
            if current_mtimes != last_mtimes:
                print(f"\n[*] Change detected, re-bundling...")
                last_mtimes = current_mtimes
                bundler.build()
    except KeyboardInterrupt:
        print("\n[*] Watch mode stopped.")

def main():
    parser = argparse.ArgumentParser(description="Bundle modular C++ sources into Windhawk monolithic mod file.")
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUTPUT, help="Path for bundled .wh.cpp")
    parser.add_argument("-s", "--src", type=Path, default=SRC_DIR, help="Source directory containing main.cpp")
    parser.add_argument("-w", "--watch", action="store_true", help="Watch source directory for file modifications")

    args = parser.parse_args()

    bundler = Bundler(args.src, args.output)
    if args.watch:
        watch_mode(bundler, args.src)
    else:
        success = bundler.build()
        sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
