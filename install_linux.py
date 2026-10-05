#!/usr/bin/env python3
"""User-local Linux installer for the exact Gradient Data customer bundle."""
import ctypes
import errno
import hashlib
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import sys
import tempfile

VERSION = "0.3.0-rc.5"
REQUIRED = {
    "linux-x86_64/uacd", "linux-x86_64/uacd-gui", "linux-x86_64/libgradient_data_c.so",
    "linux-x86_64/libgradient_data_c.a", "include/gradient_data.h",
    "include/gradient_data.hpp", "include/uacd_archive.h", "include/uacd_archive.hpp", "cmake/GradientDataConfig.cmake",
}


def sha256(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def manifest_rows(root):
    lines = (root / "SHA256SUMS").read_text("ascii").splitlines()
    rows = {}
    for line in lines:
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_./-]+)", line)
        if not match:
            raise ValueError("invalid SHA256SUMS")
        name = match[2]
        if (name in rows or name.startswith("/") or
                PurePosixPath(name).as_posix() != name or
                ".." in PurePosixPath(name).parts):
            raise ValueError("invalid manifest path")
        rows[name] = match[1]
    if not REQUIRED <= set(rows):
        raise ValueError("incomplete Linux bundle")
    return rows


def expected_directories(rows):
    return {parent.as_posix() for name in rows
            for parent in PurePosixPath(name).parents if str(parent) != "."}


def verify_tree(root, rows, installed=False):
    if root.is_symlink() or not root.is_dir():
        raise ValueError("bundle or installation is not an ordinary directory")
    files, directories = set(), set()
    for base, dirs, names in os.walk(root, followlinks=False):
        for name in dirs + names:
            path = Path(base) / name
            rel = path.relative_to(root).as_posix()
            mode = path.lstat().st_mode
            if stat.S_ISDIR(mode):
                directories.add(rel)
            elif stat.S_ISREG(mode):
                files.add(rel)
            else:
                raise ValueError(f"nonregular entry: {rel}")
    expected_files = set(rows) | {"SHA256SUMS"}
    if installed:
        expected_files.add("INSTALL_RECEIPT")
        manifest = (root / "SHA256SUMS").read_bytes()
        marker = (f"Gradient Data {VERSION}\n" +
                  hashlib.sha256(manifest).hexdigest() + "\n").encode("ascii")
        if (root / "INSTALL_RECEIPT").read_bytes() != marker:
            raise ValueError("installation receipt mismatch")
    if files != expected_files or directories != expected_directories(rows):
        raise ValueError("file/directory roster mismatch")
    for name, digest in rows.items():
        if sha256(root / name) != digest:
            raise ValueError(f"checksum mismatch: {name}")


def same_inode(path, identity, kind):
    try:
        mode = path.lstat().st_mode
        st = path.lstat()
    except FileNotFoundError:
        return False
    return (kind(mode) and (st.st_dev, st.st_ino) == identity)


def rename_noreplace(stage, prefix):
    """Linux-only atomic publish; never fall back to replacing rename/mv."""
    if sys.platform != "linux":
        raise OSError(errno.ENOSYS, "renameat2 requires Linux")
    libc = ctypes.CDLL(None, use_errno=True)
    function = getattr(libc, "renameat2", None)
    if function is None:
        raise OSError(errno.ENOSYS, "libc renameat2 is unavailable")
    function.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int,
                         ctypes.c_char_p, ctypes.c_uint]
    function.restype = ctypes.c_int
    at_fdcwd = -100
    rename_noreplace_flag = 1
    result = function(at_fdcwd, os.fsencode(stage), at_fdcwd,
                      os.fsencode(prefix), rename_noreplace_flag)
    if result != 0:
        code = ctypes.get_errno()
        if code == errno.EEXIST:
            raise FileExistsError(code, "installation prefix already exists", str(prefix))
        raise OSError(code, f"renameat2(RENAME_NOREPLACE) failed: {os.strerror(code)}", str(prefix))


def install(root, prefix, bin_dir, publish=None):
    """Prepare and verify a private stage before atomic no-replace publication."""
    rows = manifest_rows(root)
    verify_tree(root, rows)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    bin_dir.mkdir(parents=True, exist_ok=True)
    stage = None
    stage_identity = None
    created_dirs = []
    created_files = []
    published = False
    try:
        stage = Path(tempfile.mkdtemp(prefix=".gradient-data-install-", dir=prefix.parent))
        st = stage.lstat()
        stage_identity = (st.st_dev, st.st_ino)
        for name in sorted(expected_directories(rows), key=lambda item: (item.count("/"), item)):
            path = stage / name
            path.mkdir()
            st = path.lstat()
            created_dirs.append((path, (st.st_dev, st.st_ino)))
        for name in sorted(set(rows) | {"SHA256SUMS"}):
            source = root / name
            target = stage / name
            with source.open("rb") as readable, target.open("xb") as writable:
                st = os.fstat(writable.fileno())
                created_files.append((target, (st.st_dev, st.st_ino)))
                shutil.copyfileobj(readable, writable, 1024 * 1024)
            target.chmod(stat.S_IMODE(source.lstat().st_mode))
        marker = (f"Gradient Data {VERSION}\n" +
                  sha256(stage / "SHA256SUMS") + "\n").encode("ascii")
        receipt = stage / "INSTALL_RECEIPT"
        with receipt.open("xb") as writable:
            st = os.fstat(writable.fileno())
            created_files.append((receipt, (st.st_dev, st.st_ino)))
            writable.write(marker)
        verify_tree(stage, rows, installed=True)
        if publish is None:
            rename_noreplace(stage, prefix)
        else:
            publish(stage, prefix)  # Synthetic pre-publish race injection.
        published = True
    except BaseException:
        if stage is not None and stage_identity is not None and not published:
            for path, identity in reversed(created_files):
                if same_inode(path, identity, stat.S_ISREG):
                    path.unlink()
            for path, identity in reversed(created_dirs):
                if same_inode(path, identity, stat.S_ISDIR):
                    try:
                        path.rmdir()  # Unexpected content stays for inspection.
                    except OSError:
                        pass
            if same_inode(stage, stage_identity, stat.S_ISDIR):
                try:
                    stage.rmdir()
                except OSError:
                    pass
        raise
    for command in ("uacd", "uacd-gui"):
        link = bin_dir / command
        try:
            link.symlink_to(prefix / "linux-x86_64" / command)
        except FileExistsError:
            pass  # Preserve an existing user command.
        except OSError as exc:
            print(f"Installed, but user-local {command} link could not be created: {exc}", file=sys.stderr)
    return prefix


def main():
    root = Path(__file__).resolve().parent
    data_home = Path(os.environ.get("XDG_DATA_HOME", str(Path.home() / ".local/share")))
    bin_dir = Path(os.environ.get("XDG_BIN_HOME", str(Path.home() / ".local/bin")))
    prefix = data_home / "gradient-data" / VERSION
    try:
        print("Installed at", install(root, prefix, bin_dir))
    except (OSError, ValueError) as exc:
        raise SystemExit(f"installation failed: {exc}") from exc


if __name__ == "__main__":
    main()
