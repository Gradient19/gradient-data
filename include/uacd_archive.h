#ifndef UACD_ARCHIVE_H_INCLUDED
#define UACD_ARCHIVE_H_INCLUDED
/* Gradient Data indexed folder SDK, additive C ABI v1 (requires 0.3+).
 * The original gradient_data.h contracts, status codes, options and limits apply.
 * Reader/entry callbacks borrow verified data only for their call. Do not throw,
 * unwind or longjmp. Copy paths/metadata if needed later. Archive authors are not
 * authenticated. Inputs and archive bytes must remain unchanged during operations.
 * Input roots: regular files/directories only, 1..1024 UTF-8 paths; directories
 * are stored under their basename. Links/special files/nonportable names fail.
 * File bytes, empty directories and executable flags are preserved. ACLs, owner,
 * timestamps and hard-link identity are not. Destination parent must be trusted.
 * Full tree extraction is staged and published atomically without overwrite.
 * Default folder limits: 64 MiB aggregate metadata and <=100000 entries. Explicit
 * limits may tighten admission; implementation safety ceilings remain in force.
 * Handles share the library's 256 open/pending-reader budget. An archive handle
 * is immutable and can open independent member readers concurrently. Close may
 * race an ongoing call; that call retains its own reference. Member handles stay
 * valid after their archive handle closes and use all existing dgrad_reader_* APIs.
 * Errors use dgrad_last_error. Every non-null pointer must be valid and aligned;
 * scalar outputs must not alias inputs. The input array has count valid pointers.
 */
#include "gradient_data.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef uint64_t uacd_archive;
#define UACD_ENTRY_DIRECTORY 1u
#define UACD_ENTRY_EXECUTABLE 2u
#define UACD_ARCHIVE_DEFAULT_MAX_ENTRIES 100000u
typedef struct uacd_entry {
    uint32_t struct_size, abi_version, flags, reserved;
    uint64_t raw_bytes, archive_bytes;
    uint8_t raw_sha256[32]; /* zeros for directories */
} uacd_entry;
/* Path is UTF-8, length bytes, NOT NUL terminated. Return nonzero to cancel. */
typedef int32_t (*uacd_entry_fn)(void *context,const char *path,size_t length,const uacd_entry *entry);
DGRAD_API int32_t uacd_archive_pack(const char *const *inputs,size_t count,const char *output,
    const dgrad_options *options,dgrad_progress_fn progress,void *context);
/* NULL limits select folder defaults. max_entries=0 rejects any valid archive. */
DGRAD_API int32_t uacd_archive_open_with_limits(const char *input,const dgrad_read_limits *limits,
    uint64_t max_entries,uacd_archive *output);
DGRAD_API int32_t uacd_archive_close(uacd_archive archive);
DGRAD_API int32_t uacd_archive_list(uacd_archive archive,uacd_entry_fn visitor,void *context);
/* member is an exact UTF-8 slash-separated name as returned by list. */
DGRAD_API int32_t uacd_archive_member_open(uacd_archive archive,const char *member,dgrad_reader *output);
DGRAD_API int32_t uacd_archive_verify(uacd_archive archive);
DGRAD_API int32_t uacd_archive_unpack(uacd_archive archive,const char *output,
    dgrad_progress_fn progress,void *context);
DGRAD_API int32_t uacd_archive_extract(uacd_archive archive,const char *member,const char *output);
#ifdef __cplusplus
}
static_assert(sizeof(uacd_entry)==64,"uacd_entry ABI layout mismatch");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(uacd_entry)==64,"uacd_entry ABI layout mismatch");
#endif
#endif
