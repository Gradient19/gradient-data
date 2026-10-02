# Gradient Data

Gradient Data is an indexed file compression tool being prepared for Linux and Windows. It provides whole-file recovery and byte-range reads.

**UACD** means **Universal Addressable Compressed Data**. Its filename extension is **`.uacd`**. “Addressable” refers to reading a requested byte range through the file index.

## Availability

The first public evaluation package is in preparation. No binary download or installation command is available yet. This repository will contain customer documentation, installation helpers, C/C++ SDK interfaces and qualified downloadable builds.

The implementation and research material remain private. This repository is not a source-code release.

## Intended packages

- Linux x86-64 and Windows x86-64 command-line tools.
- Encoder profiles and explicit block-size settings.
- C/C++ SDK interfaces and examples for applications that need file packing or byte-range reads.

Each published build will identify its supported platforms, compatibility, checksums and known limitations. No compression advantage on every input is promised.

See [usage terms](NOTICE.md) and [security scope](SECURITY.md).
