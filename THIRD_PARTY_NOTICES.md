# Third-party notices

The distributed binaries may contain dependencies from the locked Rust build listed below. Their license texts are included verbatim under `licenses/<crate-version>/`. The list and text files are provided to preserve notices; this document is not a legal clearance opinion.

| Crate | Version | Declared license |
|---|---:|---|
| block-buffer | 0.10.4 | MIT OR Apache-2.0 |
| cfg-if | 1.0.4 | MIT OR Apache-2.0 |
| cpufeatures | 0.2.17 | MIT OR Apache-2.0 |
| crypto-common | 0.1.7 | MIT OR Apache-2.0 |
| digest | 0.10.7 | MIT OR Apache-2.0 |
| generic-array | 0.14.7 | MIT |
| itoa | 1.0.18 | MIT OR Apache-2.0 |
| libc | 0.2.186 | MIT OR Apache-2.0 |
| memchr | 2.8.2 | Unlicense OR MIT |
| proc-macro2 | 1.0.106 | MIT OR Apache-2.0 |
| quote | 1.0.46 | MIT OR Apache-2.0 |
| serde | 1.0.228 | MIT OR Apache-2.0 |
| serde_core | 1.0.228 | MIT OR Apache-2.0 |
| serde_derive | 1.0.228 | MIT OR Apache-2.0 |
| serde_json | 1.0.150 | MIT OR Apache-2.0 |
| sha2 | 0.10.9 | MIT OR Apache-2.0 |
| syn | 2.0.118 | MIT OR Apache-2.0 |
| typenum | 1.20.1 | MIT OR Apache-2.0 |
| unicode-ident | 1.0.24 | (MIT OR Apache-2.0) AND Unicode-3.0 |
| version_check | 0.9.5 | MIT OR Apache-2.0 |
| zmij | 1.0.21 | MIT |

Some listed crates are build-time dependencies; listing them conservatively does not mean every crate is present in every executable. Rust standard-library and platform runtime obligations require separate release review.

The Rust 1.98.1 standard library and runtime are distributed under the Rust project's licensing and bundled notices. Exact license texts from the installed Rust 1.98.1 toolchain are in `licenses/rust/Apache-2.0.txt`, `licenses/rust/MIT.txt`, and `licenses/rust/LLVM-exception.txt`; its exact library attribution document is `licenses/rust/COPYRIGHT-library.html`. The `unicode-ident` package's Unicode license is separately preserved at `licenses/unicode-ident-1.0.24/LICENSE-UNICODE`. Platform runtime requirements may vary by operating system and remain subject to release review.
