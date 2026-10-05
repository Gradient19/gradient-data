# Gradient Data 0.3.0-rc.5 customer bundle

Native CLI, desktop launcher and C/C++ SDK for evaluation and integration testing. Use the checksum and validation notes attached to the matching release; local Steam-workload measurements do not guarantee performance on other inputs or machines. Archive compatibility, SDK ABI and worker/memory settings are documented below.

This is an unsigned release candidate for evaluation and integration testing under [NOTICE.txt](NOTICE.txt). Download the combined Linux/Windows bundle from [Releases](https://github.com/Gradient19/gradient-data/releases) and extract it to a new directory. A source-tree checkout is not the binary bundle.

Gradient Data stores files and folders in an indexed, integrity-checked UACD (Universal Addressable Compressed Data) archive. Name new archives `.uacd`. Open the graphical interface with the supplied launcher, or run `uacd` (`uacd.exe` on Windows). See [COMPATIBILITY.md](COMPATIBILITY.md) for older archives and SDK identifiers.

This bundle contains Linux x86-64 and Windows x86-64 command-line programs, an offline graphical interface, and shared and static C/C++ SDK libraries. It does not require the private source tree. Check the publisher's authenticated release channel and published bundle digest before running it. `SHA256SUMS` detects changed files inside the bundle; it does not identify the publisher.

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
& "$env:LOCALAPPDATA\Programs\GradientData\0.3.0-rc.5\windows-x86_64\uacd.exe" --version
& "$env:LOCALAPPDATA\Programs\GradientData\0.3.0-rc.5\windows-x86_64\uacd.exe" pack input.bin output.uacd
& "$env:LOCALAPPDATA\Programs\GradientData\0.3.0-rc.5\windows-x86_64\uacd.exe" verify output.uacd
powershell -NoProfile -ExecutionPolicy Bypass -File .\uninstall.ps1
```

The Linux binaries require x86-64 and glibc 2.34 or newer. The optional Linux installer needs Python 3 and kernel/filesystem support for no-replace directory rename (`renameat2`); running the CLI directly does not require Python. Windows shared-SDK applications need the Microsoft Visual C++ runtime; the standalone CLI and supplied static SDK use a static CRT. Native CI targets Ubuntu 22.04 and Windows Server 2022; additional customer-host trials are required for deployment on other systems.

The installers use only extracted local files, check the complete listed file roster and SHA-256 hashes, and write to the user's profile. They do not download code or change the system PATH. The Linux installer provides user-local `uacd` and `uacd-gui` links in `~/.local/bin` when each name is free, and preserves an existing command or symlink. Earlier version directories and existing commands are preserved. Uninstall removes only this version and its matching command links. If the name is occupied, run `~/.local/share/gradient-data/0.3.0-rc.5/linux-x86_64/uacd` directly. If interrupted before completion, rerun it; an incomplete hidden staging directory may remain in the user data directory after forced termination, while the final versioned installation path stays free for a retry. Run directly from `linux-x86_64/uacd` or `windows-x86_64/uacd.exe` if installation is unnecessary.

`pack` accepts `--entropy off|local|shared`; the default is `off`. `local` and `shared` are optional encoding choices. A file uses container version 3 only when entropy coding actually wins; selecting entropy is not a promise of a smaller file or a new container version. `off` preserves the existing version 1/2 writing path. The single-file reader accepts all three; see [COMPATIBILITY.md](COMPATIBILITY.md) for exact headers. For example:

```sh
uacd pack input.bin output.uacd --entropy local
uacd pack input.bin output-shared.uacd --entropy shared
uacd info output.uacd
uacd range output.uacd 0 4096 first-4k.bin
```

Pack defaults to quality with 256 KiB blocks. Fast and quality accept 64, 128, 256, 512 and 1024 KiB; talon accepts up to 256 KiB. Existing destinations are never overwritten. Standalone C11 shared and C++17 static examples are in [`examples/`](examples/). See [SDK.md](SDK.md), [SECURITY.md](SECURITY.md), [COMPATIBILITY.md](COMPATIBILITY.md), and [NOTICE.txt](NOTICE.txt).

Shared entropy can use temporary space for several complete candidate archives. It is not an unconditional size or speed improvement; small files can decode more slowly. The source and archive must stay unchanged during use, and output directories must be trusted and support hard links. Single-file containers store the file's bytes. Folder/selection envelopes additionally store portable member names, empty directories and executable intent; they do not preserve ACLs, ownership, timestamps or hard-link identity.

## CPU workers and process budgets

The CLI and graphical interface default to **Auto**. Choose **Max** to target all detected available logical CPUs, or **Custom** to set a numeric worker limit. These are encoder workers, not pinned physical cores. The file format and archive bytes stay the same for the same input/settings.

```sh
uacd pack Game Game.uacd --profile fast --workers auto
uacd pack Game Game-max.uacd --profile fast --workers max
uacd pack Game Game-two.uacd --profile fast --workers 2 --memory-budget 2GiB
uacd pack Game Game-serial.uacd --workers 1
```

Numeric limits accept canonical integers from 1 through 64 and must fit detected available CPUs and memory admission. Auto leaves one logical CPU outside its worker limit when more than two are available; Max targets all detected CPUs, up to the safety ceiling. Both can select fewer workers to fit memory estimates. Short members start only useful workers. The CLI reports requested mode, detected CPU count, selected limit and reasons as `worker_selection` on stderr; the GUI shows the same selection. Detection is approximate and does not promise constant CPU utilization. An unavailable physical-memory/cgroup snapshot makes Auto select one worker; Max/custom still require the enforced process budget.

Auto, Max and numeric limits above one attach a default **2 GiB OS process budget** before source work unless `--memory-budget` supplies a different value. An explicit budget also applies with one worker; `--workers 1` without a budget retains the serial unconstrained-by-this-option path. Linux limits address space and preserves stricter inherited bounds; Windows uses startup private commit plus a conservative job committed allowance. **These are not RSS limits or memory reservations.** Admission estimates include a coordinator allowance and profile/block-dependent worker cost; physical availability is a volatile snapshot and hidden OS/container limits can still be stricter. Attachment failure is an error, with no unconstrained retry. Abrupt termination/allocation abort can leave hidden temporary files while the final archive stays unpublished.

The standalone GUI attaches its at-most 2 GiB native-process budget once before accepting jobs. An automatically launched browser starts before that new quota, and can still inherit external system limits. The SDK's existing entrypoints remain serial and never change the calling application's process quota. Larger blocks or additional workers can use more RAM; speed does not scale linearly with worker count.

## Files, folders and the graphical interface

Double-click `Start_Gradient_Data.cmd` on Windows or run `./start-gui.sh` on Linux to open the offline graphical interface in your browser. The native companion is `uacd-gui` / `uacd-gui.exe`. See [GUI.md](GUI.md) for selecting folders, profiles, progress and cancellation. Internet access is not needed for compression or decoding.

```sh
uacd pack Game Game.uacd
uacd pack-many selected.uacd textures sounds config.json --profile fast
uacd list Game.uacd
uacd verify Game.uacd
uacd unpack Game.uacd restored
uacd extract Game.uacd Game/assets/data.bin one-asset.bin
uacd range Game.uacd 4096 8192 asset-range.bin --file Game/assets/data.bin
```

Each selected input root retains its basename. `unpack` restores into a **new** output directory: the example creates `restored/Game/...`. Existing destinations are never replaced, even empty folders. A folder/multiple-file archive uses format version 4 and requires a 0.3+ reader; earlier valid single-file formats remain supported. Member containers retain the same block encoder, profile and entropy options, and selected members/ranges can be read without full restoration. Metadata limits: at most 100000 entries and 64 MiB aggregate index. Links and special files are not followed or silently skipped; unsupported portable names fail with a diagnostic. ACLs, owners, timestamps and hard-link identity are not preserved.

For runtime integration, include `uacd_archive.h` or `uacd_archive.hpp`. Open an archive, choose an exact listed member name and use the existing reader APIs on the returned member handle. All original 17 ABI functions remain; 8 new archive functions are additive. The SDK libraries and CMake imported targets are bundled, with `examples/read_member.c` and `.cpp` demonstrating actual member access. This is not transparent operating-system mounting; an application must use the API.
