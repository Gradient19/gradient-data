# Gradient Data 0.2.0-rc.1 customer bundle

This is an unsigned release candidate for evaluation and integration testing under [NOTICE.txt](NOTICE.txt). Download the combined Linux/Windows bundle from [Releases](https://github.com/Gradient19/gradient-data/releases) and extract it to a new directory. A source-tree checkout is not the binary bundle.

Gradient Data stores a file in an indexed, integrity-checked UACD (Universal Addressable Compressed Data) archive. Name new archives `.uacd`. The executable remains `dgrad`, and the C ABI v1 symbols remain `dgrad_*`. Existing `.dgrad` archives remain readable. Changing a filename does not convert archive contents.

This bundle contains Linux x86-64 and Windows x86-64 command-line programs and shared and static C/C++ SDK libraries. It does not require the private source tree. Check the publisher's authenticated release channel and published bundle digest before running it. `SHA256SUMS` detects changed files inside the bundle; it does not identify the publisher.

Linux:

```sh
./install.sh
~/.local/bin/dgrad --version
~/.local/bin/dgrad pack input.bin output.uacd --profile quality --block-size 256K
~/.local/bin/dgrad verify output.uacd
~/.local/bin/dgrad unpack output.uacd restored.bin
./uninstall.sh
```

Windows PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\install.ps1
& "$env:LOCALAPPDATA\Programs\GradientData\0.2.0-rc.1\windows-x86_64\dgrad.exe" --version
& "$env:LOCALAPPDATA\Programs\GradientData\0.2.0-rc.1\windows-x86_64\dgrad.exe" pack input.bin output.uacd
& "$env:LOCALAPPDATA\Programs\GradientData\0.2.0-rc.1\windows-x86_64\dgrad.exe" verify output.uacd
powershell -NoProfile -ExecutionPolicy Bypass -File .\uninstall.ps1
```

The Linux binaries require x86-64 and glibc 2.34 or newer. The optional Linux installer needs Python 3 and kernel/filesystem support for no-replace directory rename (`renameat2`); running the CLI directly does not require Python. Windows shared-SDK applications need the Microsoft Visual C++ runtime; the standalone CLI and supplied static SDK use a static CRT. Native qualification uses Ubuntu 22.04 and Windows Server 2022, plus a local Steam Deck/Linux trial; it does not certify every Linux distribution or Windows desktop.

The installers use only extracted local files, check the complete listed file roster and SHA-256 hashes, and write to the user's profile. They do not download code or change the system PATH. The Linux installer provides a user-local `dgrad` link in `~/.local/bin` when that name is free, and preserves an existing command. If the name is occupied, run `~/.local/share/gradient-data/0.2.0-rc.1/linux-x86_64/dgrad` directly. If interrupted before completion, rerun it; an incomplete hidden staging directory may remain in the user data directory after forced termination, while the versioned installation path stays available. Run directly from `linux-x86_64/dgrad` or `windows-x86_64/dgrad.exe` if installation is unnecessary.

`pack` accepts `--entropy off|local|shared`; the default is `off`. `local` and `shared` are optional encoding choices. A file uses UACD0003 only when entropy coding actually wins; selecting entropy is not a promise of a smaller file or a new container version. `off` preserves the existing DGRAD001/DGRAD002 writing path. The reader accepts all three. For example:

```sh
dgrad pack input.bin output.uacd --entropy local
dgrad pack input.bin output-shared.uacd --entropy shared
dgrad info output.uacd
dgrad range output.uacd 0 4096 first-4k.bin
```

Pack defaults to quality with 256 KiB blocks. Fast and quality accept 64, 128, 256, 512 and 1024 KiB; talon accepts up to 256 KiB. Existing destinations are never overwritten. Standalone C11 shared and C++17 static examples are in [`examples/`](examples/). See [SDK.md](SDK.md), [SECURITY.md](SECURITY.md), [COMPATIBILITY.md](COMPATIBILITY.md), and [NOTICE.txt](NOTICE.txt).

Shared entropy can use temporary space for several complete candidate archives. It is not an unconditional size or speed improvement; small files can decode more slowly. The source and archive must stay unchanged during use, and output directories must be trusted and support hard links. A container stores one file's bytes, not a directory tree, filenames, permissions or timestamps.
