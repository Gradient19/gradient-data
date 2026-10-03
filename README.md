# Gradient Data 0.2.0-rc.3 customer bundle

This is an unsigned release candidate for evaluation and integration testing under [NOTICE.txt](NOTICE.txt). Download the combined Linux/Windows bundle from [Releases](https://github.com/Gradient19/gradient-data/releases) and extract it to a new directory. A source-tree checkout is not the binary bundle.

Gradient Data stores a file in an indexed, integrity-checked UACD (Universal Addressable Compressed Data) archive. Name new archives `.uacd`. Run the `uacd` executable (`uacd.exe` on Windows). See [COMPATIBILITY.md](COMPATIBILITY.md) for older archives and SDK identifiers.

This bundle contains Linux x86-64 and Windows x86-64 command-line programs and shared and static C/C++ SDK libraries. It does not require the private source tree. Check the publisher's authenticated release channel and published bundle digest before running it. `SHA256SUMS` detects changed files inside the bundle; it does not identify the publisher.

Linux:

```sh
./install.sh
~/.local/bin/uacd --version
~/.local/bin/uacd pack input.bin output.uacd --profile quality --block-size 256K
~/.local/bin/uacd verify output.uacd
~/.local/bin/uacd unpack output.uacd restored.bin
./uninstall.sh
```

Windows PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\install.ps1
& "$env:LOCALAPPDATA\Programs\GradientData\0.2.0-rc.3\windows-x86_64\uacd.exe" --version
& "$env:LOCALAPPDATA\Programs\GradientData\0.2.0-rc.3\windows-x86_64\uacd.exe" pack input.bin output.uacd
& "$env:LOCALAPPDATA\Programs\GradientData\0.2.0-rc.3\windows-x86_64\uacd.exe" verify output.uacd
powershell -NoProfile -ExecutionPolicy Bypass -File .\uninstall.ps1
```

The Linux binaries require x86-64 and glibc 2.34 or newer. The optional Linux installer needs Python 3 and kernel/filesystem support for no-replace directory rename (`renameat2`); running the CLI directly does not require Python. Windows shared-SDK applications need the Microsoft Visual C++ runtime; the standalone CLI and supplied static SDK use a static CRT. Native CI targets Ubuntu 22.04 and Windows Server 2022; additional customer-host trials are required for deployment on other systems.

The installers use only extracted local files, check the complete listed file roster and SHA-256 hashes, and write to the user's profile. They do not download code or change the system PATH. The Linux installer provides a user-local `uacd` link in `~/.local/bin` when that name is free, and preserves an existing command or symlink. Earlier version directories and existing commands are preserved. Uninstall removes only this version and its matching `uacd` link. If the name is occupied, run `~/.local/share/gradient-data/0.2.0-rc.3/linux-x86_64/uacd` directly. If interrupted before completion, rerun it; an incomplete hidden staging directory may remain in the user data directory after forced termination, while the final versioned installation path stays free for a retry. Run directly from `linux-x86_64/uacd` or `windows-x86_64/uacd.exe` if installation is unnecessary.

`pack` accepts `--entropy off|local|shared`; the default is `off`. `local` and `shared` are optional encoding choices. A file uses container version 3 only when entropy coding actually wins; selecting entropy is not a promise of a smaller file or a new container version. `off` preserves the existing version 1/2 writing path. The reader accepts all three; see [COMPATIBILITY.md](COMPATIBILITY.md) for exact headers. For example:

```sh
uacd pack input.bin output.uacd --entropy local
uacd pack input.bin output-shared.uacd --entropy shared
uacd info output.uacd
uacd range output.uacd 0 4096 first-4k.bin
```

Pack defaults to quality with 256 KiB blocks. Fast and quality accept 64, 128, 256, 512 and 1024 KiB; talon accepts up to 256 KiB. Existing destinations are never overwritten. Standalone C11 shared and C++17 static examples are in [`examples/`](examples/). See [SDK.md](SDK.md), [SECURITY.md](SECURITY.md), [COMPATIBILITY.md](COMPATIBILITY.md), and [NOTICE.txt](NOTICE.txt).

Shared entropy can use temporary space for several complete candidate archives. It is not an unconditional size or speed improvement; small files can decode more slowly. The source and archive must stay unchanged during use, and output directories must be trusted and support hard links. A container stores one file's bytes, not a directory tree, filenames, permissions or timestamps.
