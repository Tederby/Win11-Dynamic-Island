#!/usr/bin/env python3
"""
Windhawk Mod Verification Script
Compiles and links the bundled mod using Windhawk's native Clang toolchain
for both i686 (32-bit) and x86_64 (64-bit) targets to catch compiler and
linker issues before deploying or presenting to the user.
"""

import os
import sys
import re
import subprocess
import tempfile
import shutil
from pathlib import Path

# Possible Windhawk installation paths
POSSIBLE_COMPILER_PATHS = [
    Path(r"D:\Program Files\Windhawk\Compiler"),
    Path(r"C:\Program Files\Windhawk\Compiler"),
    Path(os.environ.get("PROGRAMFILES", "C:\\Program Files")) / "Windhawk" / "Compiler",
]

def find_windhawk_compiler():
    for base in POSSIBLE_COMPILER_PATHS:
        clang_path = base / "bin" / "clang++.exe"
        header_path = base / "include" / "windhawk_api.h"
        if clang_path.exists() and header_path.exists():
            return clang_path, header_path
    
    # Try finding in PATH
    clang_in_path = shutil.which("clang++")
    if clang_in_path:
        return Path(clang_in_path), None
    return None, None

def extract_compiler_options(wh_cpp_path):
    with open(wh_cpp_path, "r", encoding="utf-8", errors="replace") as f:
        content = f.read(4096)
    
    match = re.search(r"//\s*@compilerOptions\s+(.+)$", content, re.MULTILINE)
    if match:
        return match.group(1).strip().split()
    
    # Default fallback
    return [
        "-ld2d1", "-ldwrite", "-lwindowscodecs", "-luxtheme",
        "-lole32", "-loleaut32", "-lruntimeobject", "-lwindowsapp",
        "-lshcore", "-lversion", "-lgdi32", "-ldwmapi",
        "-luser32", "-lshell32", "-ladvapi32"
    ]

def verify_target(clang_exe, header_path, wh_cpp_path, target, extra_options):
    print(f"[*] Testing target: {target} ...", end=" ", flush=True)
    temp_dir = Path(tempfile.gettempdir())
    out_dll = temp_dir / f"windhawk_verify_{target}.dll"
    
    cmd = [
        str(clang_exe),
        "-shared",
        "-target", target,
        "-std=c++20",
        "-DUNICODE",
        "-D_UNICODE",
    ]
    
    if header_path:
        cmd.extend(["-include", str(header_path)])
    
    cmd.append(str(wh_cpp_path))
    cmd.extend(extra_options)
    cmd.extend(["-o", str(out_dll)])
    
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if out_dll.exists():
        try:
            out_dll.unlink()
        except OSError:
            pass
            
    if res.returncode == 0:
        print("OK (Pass)")
        return True, ""
    else:
        print("FAILED")
        error_msg = res.stderr.strip() or res.stdout.strip()
        return False, error_msg

def main():
    repo_root = Path(__file__).resolve().parent.parent
    wh_cpp = repo_root / "win11-dynamic-island.wh.cpp"
    
    if not wh_cpp.exists():
        print(f"[!] {wh_cpp.name} not found. Running bundler first...")
        bundle_script = repo_root / "scripts" / "bundle.py"
        subprocess.run([sys.executable, str(bundle_script)], check=True)
    
    clang_exe, header_path = find_windhawk_compiler()
    if not clang_exe:
        print("[!] Windhawk Compiler (clang++.exe) not found in standard directories.")
        print("[!] Please ensure Windhawk is installed or add clang++ to PATH.")
        return 1
        
    print(f"[+] Using Windhawk Clang: {clang_exe}")
    if header_path:
        print(f"[+] Using Windhawk API:   {header_path}")
        
    compiler_options = extract_compiler_options(wh_cpp)
    print(f"[+] Loaded @compilerOptions ({len(compiler_options)} flags): {' '.join(compiler_options)}")
    
    targets = ["x86_64-w64-windows-gnu", "i686-w64-windows-gnu"]
    all_passed = True
    
    for t in targets:
        passed, err = verify_target(clang_exe, header_path, wh_cpp, t, compiler_options)
        if not passed:
            all_passed = False
            print(f"\n--- Compilation / Linker Output for {t} ---")
            print(err)
            print("-" * 50)
            
    if all_passed:
        print("\n[+] Verification SUCCESS: All targets compiled and linked cleanly with 0 errors!")
        return 0
    else:
        print("\n[!] Verification FAILED: Resolve the errors above and refer to docs/COMPILER_GUIDE.md")
        return 1

if __name__ == "__main__":
    sys.exit(main())
