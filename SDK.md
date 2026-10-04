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
