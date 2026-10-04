# C and C++ SDK

Include `include/gradient_data.h` from C11 or `include/gradient_data.hpp` from C++17. The bundle has matching shared and static `gradient_data_c` libraries for Linux x86-64 and Windows x86-64. The C++ wrapper uses the same C ABI v1. Its original 17 export names, struct sizes (32/40/80 bytes), and existing function signatures remain stable within ABI v1. Use the bundled header and library together.

Call `dgrad_options_init` before setting `dgrad_options.flags`: `0` disables entropy, `1` requests local entropy, and `2` requests shared entropy. These are exclusive values. The default is `0`. The packer may retain the earlier archive format when entropy coding does not win. The raw-block API interprets shared as self-contained local entropy; it does not require state from another block. A raw encoded block is not a complete `.uacd` file.

Use `dgrad_read_limits_init` before setting reader budgets. Caller buffers remain caller owned; borrowed range slices are valid only during their callback. Copy thread-local error text before another status-returning call on the same thread.

With CMake 3.16 or newer, point `GradientData_DIR` at this bundle's `cmake` directory:

```cmake
find_package(GradientData CONFIG REQUIRED)
add_executable(reader_c reader.c)
target_link_libraries(reader_c PRIVATE GradientData::shared)
add_executable(reader_cpp reader.cpp)
target_link_libraries(reader_cpp PRIVATE GradientData::static)
if(MSVC)
  set_property(TARGET reader_cpp PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()
```

`GradientData::gradient_data` remains a compatibility alias for the shared target. The imported targets supply the include directory and correct platform library. `GradientData::static` defines `DGRAD_STATIC` for consumers. On Linux it also carries the platform thread/dynamic-loader/math link dependencies. On Windows the supplied static library was built with the static MSVC runtime, so build its consumer with `/MT` as shown and use a compatible toolset. The shared DLL uses the dynamic MSVC runtime and its `.dll.lib` import library; deploy the DLL beside a shared consumer or on a controlled DLL search path. Static consumers do not need that DLL. Never link both variants into one process.

Configure the included standalone examples with `cmake -S examples -B build -DGradientData_DIR=/path/to/bundle/cmake`, then `cmake --build build --config Release`. Run `read_range_c FILE.uacd` (shared) and `read_range_cpp FILE.uacd` (static). These examples use only the public headers and the imported targets.

For direct Linux shared linking, add `include` to the compiler include path, `linux-x86_64` to the library search path, link `-lgradient_data_c`, and configure an appropriate runtime loader path. For direct Linux static linking, name `linux-x86_64/libgradient_data_c.a` and include the platform thread, `dl`, `m`, `rt`, and `util` link libraries. On MSVC, link `windows-x86_64/gradient_data_c.dll.lib` for shared or `windows-x86_64/gradient_data_c.lib` with `/MT` and `DGRAD_STATIC` for static. Direct static links also need `advapi32`, `userenv`, `ws2_32`, `bcrypt`, `ntdll`, `kernel32`, and `synchronization`; the imported target carries these dependencies. Do not mix platform libraries or architectures.

## Indexed folder archives (0.3+)

Include `uacd_archive.h` for C11 or `uacd_archive.hpp` for C++17, and link the same shared/static library. These headers add 8 archive functions without changing the original 17 functions or ABI v1 layouts. `uacd_archive_open_with_limits` validates the path index and all member indexes; NULL limits use 64 MiB aggregate metadata and 100000 entries. Set `max_entries` explicitly (normally `UACD_ARCHIVE_DEFAULT_MAX_ENTRIES`). The implementation ceilings remain in force even if a caller raises its limits.

`uacd_archive_list` visits borrowed UTF-8 relative paths and metadata; paths are length-delimited, not NUL terminated. `uacd_archive_member_open` returns a normal `dgrad_reader`, so existing read-at, visit, info, verify and close functions work on a selected file. Member handles retain their file reference when the archive handle closes. Independent member handles can read concurrently. The library-wide 256 open/pending-handle cap includes both archives and members.

`uacd_archive_pack` handles 1..1024 file/directory roots and existing encoding options. `uacd_archive_unpack` stages the entire validated tree and publishes a new directory without overwrite; progress callbacks can cancel between files/blocks. `uacd_archive_extract` restores one file. No directory-tree mounting, asset-loader integration or 7-Zip plugin is implied; adapt your application using these APIs. Existing readers from 0.2 cannot open version 4 envelopes, while single-file archive compatibility is retained.

See `examples/read_member.c` and `examples/read_member.cpp`, built through the supplied CMake package. Their arguments are ARCHIVE MEMBER, and they write the first up to 4096 verified bytes to stdout. Windows paths supplied to the C API are UTF-8.

## Bounded runtime asset reader

`examples/runtime_asset_reader.c` builds as `runtime_asset_reader_c` (shared C11) and `runtime_asset_reader_cpp` (static C++17) through the same CMake package. Both use only the existing C ABI. Run either executable with `archive ARCHIVE.uacd MEMBER [OFFSET LENGTH]` or use `loose FILE [OFFSET LENGTH]` for the corresponding loose-file control. The exact member name is the UTF-8 slash-separated name returned by the archive list API. Windows command-line arguments are converted from native UTF-16 to strict UTF-8.

Without a range, the example reads the first, middle and tail windows, each at most 4096 bytes. An explicit decimal range is limited to 65536 bytes and must fit entirely within the member or loose file; a zero-length range at its end is allowed. The sample archive admission policy caps aggregate raw bytes at 64 GiB, metadata at 64 MiB, archive bytes at 128 GiB and entries at 100000. These are application admission limits, not OS memory or time quotas. The member reader stays alive after its parent archive closes. Each archive range read verifies the touched codec blocks; the example calls no full-archive/member verification or unpack operation. Work within a touched block can exceed the requested range.

The executable prints JSON with numeric status, range dimensions, SHA-256 digests, and monotonic timings; asset contents and private paths are omitted. The loose-file mode performs the same window reads and hashing. `open_ms` includes opening, metadata/member setup and parent archive closure; each `read_ms` excludes hashing, while `total_ms_including_hashes` includes it and cleanup. Cache state is reported as unmanaged: repeating a run does not establish a warm or cold cache. Process I/O counters cover opening, reads, hashes and closure. Linux `getrusage_ru_inblock` counts filesystem input operations; Windows read-operation and read-transfer counters describe process I/O, including transfer bytes. They do not establish physical disk bytes or prove that only the requested bytes were accessed. Results describe this standalone consumer and its inputs, not an installed game integration or a general performance guarantee.
