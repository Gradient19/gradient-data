# Security scope

UACD archive hashes detect accidental corruption and inconsistent data. They are not publisher signatures, encryption, access control, or protection against a malicious archive author. Check the release through an authenticated publisher channel before running it; a matching `SHA256SUMS` file alone does not establish origin.

Use a trusted output directory that supports hard links. Keep the input or archive unchanged during an operation. An archive can request substantial disk, memory, and CPU work; inspect metadata and set `--max-output` and `--max-index` where applicable. Do not treat the limits as operating-system quotas. Existing output names are not replaced, and forced termination can leave partial files.

Only Linux x86-64 and Windows x86-64 are intended for this bundle. A checksum check does not establish security audit, compatibility on every host, or freedom from dependency vulnerabilities. Report suspected security issues through this repository's Security → Report a vulnerability flow: https://github.com/Gradient19/gradient-data/security/advisories/new . Do not include sensitive archives in a public issue. Regular bugs can be filed at https://github.com/Gradient19/gradient-data/issues with the version, platform and a small non-sensitive reproduction.

Release builds omit developer test tools and remove descriptive first-party build metadata. Static SDK libraries retain the public ABI and the relocation information required for linking. This cleanup is not encryption, licensing enforcement or prevention of binary copying and reverse engineering.
