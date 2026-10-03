#!/bin/sh
set -eu
prefix=${XDG_DATA_HOME:-"$HOME/.local/share"}/gradient-data/0.2.0-rc.1
bin_dir=${XDG_BIN_HOME:-"$HOME/.local/bin"}
command -v python3 >/dev/null 2>&1 || { echo 'python3 is required for exact uninstall verification' >&2; exit 1; }
python3 - "$prefix" "$bin_dir" <<'PY'
import hashlib, os, pathlib, re, stat, sys
root = pathlib.Path(sys.argv[1])
link = pathlib.Path(sys.argv[2]) / 'dgrad'
if root.is_symlink() or not root.is_dir():
    raise SystemExit('matching installation not found')
manifest_path = root / 'SHA256SUMS'
manifest = manifest_path.read_bytes()
receipt = (root / 'INSTALL_RECEIPT').read_text('ascii').splitlines()
if receipt != ['Gradient Data 0.2.0-rc.1', hashlib.sha256(manifest).hexdigest()]:
    raise SystemExit('installation receipt mismatch')
rows = {}
for line in manifest.decode('ascii').splitlines():
    match = re.fullmatch(r'([0-9a-f]{64})  ([A-Za-z0-9_./-]+)', line)
    if not match:
        raise SystemExit('invalid installed manifest')
    name = match[2]
    if name in rows or pathlib.PurePosixPath(name).as_posix() != name or name.startswith('/') or '..' in pathlib.PurePosixPath(name).parts:
        raise SystemExit('invalid installed manifest path')
    rows[name] = match[1]
expected_dirs = {str(parent) for name in rows for parent in pathlib.PurePosixPath(name).parents if str(parent) != '.'}
actual_files, actual_dirs = set(), set()
for base, dirs, files in os.walk(root, followlinks=False):
    for name in dirs + files:
        path = pathlib.Path(base) / name
        rel = path.relative_to(root).as_posix()
        mode = path.lstat().st_mode
        if stat.S_ISDIR(mode):
            actual_dirs.add(rel)
        elif stat.S_ISREG(mode):
            actual_files.add(rel)
        else:
            raise SystemExit('nonregular installed entry: ' + rel)
if actual_dirs != expected_dirs or actual_files != set(rows) | {'SHA256SUMS', 'INSTALL_RECEIPT'}:
    raise SystemExit('installation contains unexpected files or directories')
for name, expected in rows.items():
    digest = hashlib.sha256()
    with (root / name).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    if digest.hexdigest() != expected:
        raise SystemExit('installed file modified: ' + name)
if link.is_symlink() and os.readlink(link) == str(root / 'linux-x86_64/dgrad'):
    link.unlink()
for name in sorted(rows, key=lambda part: (part.count('/'), part), reverse=True):
    (root / name).unlink()
(root / 'INSTALL_RECEIPT').unlink()
manifest_path.unlink()
for name in sorted(expected_dirs, key=lambda part: (part.count('/'), part), reverse=True):
    (root / name).rmdir()
root.rmdir()
print('Removed', root)
PY
