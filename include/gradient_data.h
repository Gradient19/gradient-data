#ifndef GRADIENT_DATA_H_INCLUDED
#define GRADIENT_DATA_H_INCLUDED
/* Gradient Data C ABI v1 — customer evaluation SDK, not a 7-Zip plugin.
 * This header declares the interface; link libgradient_data_c as well.
 * All lengths/capacities are bytes. C/C++ callers own every supplied buffer.
 * Every non-null pointer must be valid, correctly aligned, and remain alive for
 * the call. Output scalar/struct pointers must not alias inputs or data buffers.
 * A nonzero byte length requires a non-null pointer. Zero-length data may be NULL.
 * Paths are NUL-terminated UTF-8. The CLI separately supports native OS paths.
 * No exceptions, Rust panics or longjmp may escape callbacks. Unwinding Rust
 * panics are caught; invalid pointers, OOM/abort and process signals are not.
 * Functions are synchronous. Error text is thread-local, replaced by the next
 * status-returning call on that thread. Copy it before another SDK call.
 * Archive and input files must not be modified concurrently, including by callbacks.
 * Relative pack paths are fixed before the first progress callback.
 * Concurrent process-wide cwd changes during API entry are not supported.
 * Destination directories must be trusted; publication requires hard-link support.
 * Neither integrity hashes nor this API authenticate untrusted archive authors.
 */
#include <stdint.h>
#include <stddef.h>
#ifdef _WIN32
# if defined(DGRAD_STATIC)
#  define DGRAD_API
# else
#  define DGRAD_API __declspec(dllimport)
# endif
#else
# define DGRAD_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define DGRAD_ABI_VERSION 1u
#define DGRAD_PROFILE_FAST 0u
#define DGRAD_PROFILE_QUALITY 1u
#define DGRAD_PROFILE_TALON 2u
/* options.flags: choose one value; these are modes, not combinable bit flags.
 * Zero preserves legacy bytes. New modes require version 0.2 or later. */
#define DGRAD_ENTROPY_OFF 0u
#define DGRAD_ENTROPY_LOCAL 1u
#define DGRAD_ENTROPY_SHARED 2u
#define DGRAD_BLOCK_DEFAULT 262144u
#define DGRAD_BLOCK_MAX 1048576u
/* Supported: 65536, 131072, 262144, 524288, 1048576. No arbitrary-size slider. */
enum dgrad_status {
    DGRAD_OK=0, DGRAD_INVALID_ARGUMENT=1, DGRAD_IO=2, DGRAD_INVALID_DATA=3,
    DGRAD_BUFFER_TOO_SMALL=4, DGRAD_BAD_HANDLE=5, DGRAD_BUSY=6,
    DGRAD_CANCELLED=7, DGRAD_PANIC=8, DGRAD_LIMIT=9
};
typedef uint64_t dgrad_reader;
/* Initialize using dgrad_options_init(&o,sizeof o). Reserved fields must be zero. */
typedef struct dgrad_options {
    uint32_t struct_size, abi_version, profile, block_bytes, flags, reserved[3];
} dgrad_options;
/* Optional admission policy, checked BEFORE scanning the metadata index.
 * Init defaults: unlimited file/index/archive bytes, codec block ceiling 1 MiB.
 * Zero byte ceilings permit only dimensions <= 0; max_block_bytes must be 1..1MiB.
 * Not an OS-enforced RAM/CPU quota. A policy refusal returns DGRAD_LIMIT.
 * Older libraries may not export these additive ABI v1 functions. */
typedef struct dgrad_read_limits {
    uint32_t struct_size, abi_version, max_block_bytes, reserved;
    uint64_t max_raw_bytes, max_index_bytes, max_archive_bytes;
} dgrad_read_limits;
typedef struct dgrad_info {
    uint32_t struct_size, abi_version, format_version, block_bytes;
    uint64_t raw_bytes, blocks, archive_bytes, index_offset;
    uint8_t raw_sha256[32];
} dgrad_info;
/* Return nonzero to cancel. Progress occurs before packing and between blocks,
 * not inside a long-running TALON block. raw_bytes/blocks are completed input.
 * During an optional entropy pass the completed counts remain at their final
 * values; repeated callbacks still allow cancellation before publication. */
typedef int32_t (*dgrad_progress_fn)(void *context,uint64_t raw_bytes,uint64_t blocks);
/* Data is verified for this block and borrowed only for this callback duration.
 * No extra request-sized copy by the SDK. Decoding still allocates/writes buffers.
 * Calling a read/info/verify on this SAME handle inside the callback returns BUSY.
 * Independent handles may be used concurrently. Close releases the caller handle;
 * an already-running call retains its own reference until completion. */
typedef int32_t (*dgrad_visit_fn)(void *context,const uint8_t *bytes,size_t length);
DGRAD_API uint32_t dgrad_abi_version(void);
DGRAD_API const char *dgrad_version_string(void);
/* Returns capacity needed INCLUDING NUL. Truncates and NUL-terminates if possible.
 * NULL/0 queries capacity. It does not reset the last error. */
DGRAD_API size_t dgrad_last_error(char *output,size_t capacity);
DGRAD_API int32_t dgrad_options_init(dgrad_options *output,size_t capacity);
/* NULL options select quality/256 KiB/entropy off. Existing outputs are never overwritten.
 * Fast/quality support 64/128/256/512/1024 KiB; TALON supports <= 256 KiB. */
DGRAD_API int32_t dgrad_pack_file(const char *input,const char *output,
    const dgrad_options *options,dgrad_progress_fn progress,void *context);
/* max_output=UINT64_MAX disables the caller's additional output-byte ceiling. */
DGRAD_API int32_t dgrad_unpack_file(const char *input,const char *output,uint64_t max_output);
DGRAD_API int32_t dgrad_reader_open(const char *input,dgrad_reader *output);
DGRAD_API int32_t dgrad_read_limits_init(dgrad_read_limits *output,size_t capacity);
DGRAD_API int32_t dgrad_reader_open_with_limits(const char *input,
    const dgrad_read_limits *limits,dgrad_reader *output);
DGRAD_API int32_t dgrad_reader_close(dgrad_reader reader);
DGRAD_API int32_t dgrad_reader_info(dgrad_reader reader,dgrad_info *output,size_t capacity);
DGRAD_API int32_t dgrad_reader_verify(dgrad_reader reader);
/* Open checks metadata, not every payload. Range reads verify touched blocks only.
 * A later error may leave a VERIFIED PREFIX in output; written reports its length.
 * Caller must check status. Out-of-bounds ranges fail, never silently truncate. */
DGRAD_API int32_t dgrad_reader_read_at(dgrad_reader reader,uint64_t offset,size_t length,
    uint8_t *output,size_t capacity,size_t *written);
DGRAD_API int32_t dgrad_reader_visit(dgrad_reader reader,uint64_t offset,uint64_t length,
    dgrad_visit_fn visitor,void *context);
/* Raw block API: caller stores codec byte and original length separately.
 * It is NOT a complete .uacd file. Include your own trusted integrity metadata.
 * encode_bound returns raw length; capacity >= bound is required before encoding.
 * Both entropy modes use self-contained local models for this block API; shared
 * file models are used only by dgrad_pack_file. A codec byte 'e' requires a 0.2+ reader.
 * These calls copy only after successful full-block encode/decode, written=0 on error. */
DGRAD_API int32_t dgrad_encode_bound(size_t raw_length,size_t *bound);
DGRAD_API int32_t dgrad_encode_block(const uint8_t *input,size_t input_length,
    const dgrad_options *options,uint8_t *output,size_t capacity,size_t *written,uint8_t *codec);
DGRAD_API int32_t dgrad_decode_block(uint8_t codec,const uint8_t *input,size_t input_length,
    size_t original_length,uint8_t *output,size_t capacity,size_t *written);
#ifdef __cplusplus
}
static_assert(sizeof(dgrad_options)==32,"dgrad_options ABI layout mismatch");
static_assert(sizeof(dgrad_read_limits)==40,"dgrad_read_limits ABI layout mismatch");
static_assert(sizeof(dgrad_info)==80,"dgrad_info ABI layout mismatch");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(dgrad_options)==32,"dgrad_options ABI layout mismatch");
_Static_assert(sizeof(dgrad_read_limits)==40,"dgrad_read_limits ABI layout mismatch");
_Static_assert(sizeof(dgrad_info)==80,"dgrad_info ABI layout mismatch");
#endif
#endif
