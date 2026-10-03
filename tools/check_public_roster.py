#!/usr/bin/env python3
"""Reject unintended tracked files and changed release text in the public repository."""
from __future__ import annotations

import hashlib
from pathlib import Path
import re
import stat
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ALLOWED = frozenset([
  ".github/workflows/customer-smoke.yml",
  ".github/workflows/public-roster.yml",
  ".gitignore",
  "COMPATIBILITY.md",
  "NOTICE.txt",
  "README.md",
  "SDK.md",
  "SECURITY.md",
  "SHA256SUMS",
  "THIRD_PARTY_NOTICES.md",
  "cmake/GradientDataConfig.cmake",
  "examples/CMakeLists.txt",
  "examples/read_range.c",
  "examples/read_range.cpp",
  "include/gradient_data.h",
  "include/gradient_data.hpp",
  "install.ps1",
  "install.sh",
  "install_linux.py",
  "licenses/block-buffer-0.10.4/LICENSE-APACHE",
  "licenses/block-buffer-0.10.4/LICENSE-MIT",
  "licenses/cfg-if-1.0.4/LICENSE-APACHE",
  "licenses/cfg-if-1.0.4/LICENSE-MIT",
  "licenses/cpufeatures-0.2.17/LICENSE-APACHE",
  "licenses/cpufeatures-0.2.17/LICENSE-MIT",
  "licenses/crypto-common-0.1.7/LICENSE-APACHE",
  "licenses/crypto-common-0.1.7/LICENSE-MIT",
  "licenses/digest-0.10.7/LICENSE-APACHE",
  "licenses/digest-0.10.7/LICENSE-MIT",
  "licenses/generic-array-0.14.7/LICENSE",
  "licenses/itoa-1.0.18/LICENSE-APACHE",
  "licenses/itoa-1.0.18/LICENSE-MIT",
  "licenses/libc-0.2.186/LICENSE-APACHE",
  "licenses/libc-0.2.186/LICENSE-MIT",
  "licenses/memchr-2.8.2/COPYING",
  "licenses/memchr-2.8.2/LICENSE-MIT",
  "licenses/memchr-2.8.2/UNLICENSE",
  "licenses/proc-macro2-1.0.106/LICENSE-APACHE",
  "licenses/proc-macro2-1.0.106/LICENSE-MIT",
  "licenses/quote-1.0.46/LICENSE-APACHE",
  "licenses/quote-1.0.46/LICENSE-MIT",
  "licenses/rust/Apache-2.0.txt",
  "licenses/rust/COPYRIGHT-library.html",
  "licenses/rust/LLVM-exception.txt",
  "licenses/rust/MIT.txt",
  "licenses/serde-1.0.228/LICENSE-APACHE",
  "licenses/serde-1.0.228/LICENSE-MIT",
  "licenses/serde_core-1.0.228/LICENSE-APACHE",
  "licenses/serde_core-1.0.228/LICENSE-MIT",
  "licenses/serde_derive-1.0.228/LICENSE-APACHE",
  "licenses/serde_derive-1.0.228/LICENSE-MIT",
  "licenses/serde_json-1.0.150/LICENSE-APACHE",
  "licenses/serde_json-1.0.150/LICENSE-MIT",
  "licenses/sha2-0.10.9/LICENSE-APACHE",
  "licenses/sha2-0.10.9/LICENSE-MIT",
  "licenses/syn-2.0.118/LICENSE-APACHE",
  "licenses/syn-2.0.118/LICENSE-MIT",
  "licenses/typenum-1.20.1/LICENSE",
  "licenses/typenum-1.20.1/LICENSE-APACHE",
  "licenses/typenum-1.20.1/LICENSE-MIT",
  "licenses/unicode-ident-1.0.24/LICENSE-APACHE",
  "licenses/unicode-ident-1.0.24/LICENSE-MIT",
  "licenses/unicode-ident-1.0.24/LICENSE-UNICODE",
  "licenses/version_check-0.9.5/LICENSE-APACHE",
  "licenses/version_check-0.9.5/LICENSE-MIT",
  "licenses/zmij-1.0.21/LICENSE-MIT",
  "tools/check_public_roster.py",
  "tools/customer_smoke.py",
  "uninstall.ps1",
  "uninstall.sh"
])
REPOSITORY_ONLY = frozenset({
    ".gitignore", ".github/workflows/customer-smoke.yml",
    ".github/workflows/public-roster.yml",
    "tools/customer_smoke.py", "tools/check_public_roster.py",
})
RELEASE_BINARIES = frozenset({
    "linux-x86_64/uacd", "linux-x86_64/libgradient_data_c.so",
    "linux-x86_64/libgradient_data_c.a", "windows-x86_64/uacd.exe",
    "windows-x86_64/gradient_data_c.dll", "windows-x86_64/gradient_data_c.dll.lib",
    "windows-x86_64/gradient_data_c.lib",
})
TOKEN = re.compile(rb"(?:gh[pousr]_[A-Za-z0-9]{25,}|github_pat_[A-Za-z0-9_]{25,}|-----BEGIN (?:[A-Z]+ )?PRIVATE KEY-----)")


def check():
    rows = subprocess.check_output(["git", "ls-files", "--stage", "-z"], cwd=ROOT).split(b"\0")
    tracked = {}
    for row in rows:
        if not row:
            continue
        info, path = row.split(b"\t", 1)
        mode, _sha, stage = info.split()
        name = path.decode("utf-8", "strict")
        if mode not in {b"100644", b"100755"} or stage != b"0" or name in tracked:
            raise ValueError("only ordinary resolved tracked files are permitted")
        tracked[name] = mode
    if set(tracked) != ALLOWED:
        raise ValueError(f"public file roster mismatch: extra={sorted(set(tracked) - ALLOWED)}, missing={sorted(ALLOWED - set(tracked))}")
    manifest = ROOT / "SHA256SUMS"
    records = {}
    lines = manifest.read_bytes().decode("ascii", "strict").splitlines(keepends=True)
    for line in lines:
        if not re.fullmatch(r"[0-9a-f]{64}  [^\r\n]+\n", line):
            raise ValueError("noncanonical manifest")
        digest, name = line[:-1].split("  ", 1)
        if name in records:
            raise ValueError("duplicate manifest path")
        records[name] = digest
    release_text = ALLOWED - REPOSITORY_ONLY - {"SHA256SUMS"}
    if set(records) != release_text | RELEASE_BINARIES or list(records) != sorted(records):
        raise ValueError("release manifest roster mismatch")
    for name in sorted(ALLOWED):
        path = ROOT / name
        if not stat.S_ISREG(path.lstat().st_mode) or path.is_symlink():
            raise ValueError(f"public file must be ordinary: {name}")
        data = path.read_bytes()
        if b"\0" in data:
            raise ValueError(f"unexpected binary data in tracked text: {name}")
        data.decode("utf-8", "strict")
        if TOKEN.search(data):
            raise ValueError(f"credential or private-key pattern in tracked file: {name}")
        if name in release_text and hashlib.sha256(data).hexdigest() != records[name]:
            raise ValueError(f"tracked release text differs from manifest: {name}")
    print(f"PASS: exact {len(ALLOWED)} public tracked files; {len(release_text)} release text hashes")


if __name__ == "__main__":
    try:
        check()
    except (OSError, ValueError, UnicodeError, subprocess.SubprocessError) as error:
        raise SystemExit(f"public projection rejected: {error}")
