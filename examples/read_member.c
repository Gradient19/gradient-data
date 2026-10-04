#include "uacd_archive.h"
#include <stdio.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#endif
/* Usage: read_member ARCHIVE.uacd MEMBER. Writes up to 4096 verified member
 * bytes to stdout. The member reader remains usable after its archive closes. */
#ifdef _WIN32
static int utf8_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s ARCHIVE.uacd MEMBER\n", argv[0]);
        return 2;
    }
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
        fprintf(stderr, "could not set binary stdout\n");
        return 1;
    }
#endif
    uacd_archive archive = 0;
    dgrad_reader member = 0;
    dgrad_info info = {0};
    uint8_t bytes[4096];
    size_t written = 0;
    int32_t status = uacd_archive_open_with_limits(argv[1], NULL,
        UACD_ARCHIVE_DEFAULT_MAX_ENTRIES, &archive);
    if (!status) status = uacd_archive_verify(archive);
    if (!status) status = uacd_archive_member_open(archive, argv[2], &member);
    if (!status) {
        status = uacd_archive_close(archive);
        archive = 0;
    }
    if (!status) status = dgrad_reader_verify(member);
    if (!status) status = dgrad_reader_info(member, &info, sizeof info);
    if (!status) {
        size_t length = info.raw_bytes < sizeof bytes ? (size_t)info.raw_bytes : sizeof bytes;
        status = dgrad_reader_read_at(member, 0, length, bytes, sizeof bytes, &written);
    }
    if (status) {
        char error[512];
        (void)dgrad_last_error(error, sizeof error);
        fprintf(stderr, "%s\n", error);
    } else if (fwrite(bytes, 1, written, stdout) != written || fflush(stdout) != 0) {
        fprintf(stderr, "could not write member bytes\n");
        status = DGRAD_IO;
    }
    if (member) (void)dgrad_reader_close(member);
    if (archive) (void)uacd_archive_close(archive);
    return status ? 1 : 0;
}

#ifdef _WIN32
/* Native Windows argv is UTF-16; the SDK paths are strict UTF-8. Include
 * the terminator in both passes and bound work by the command-line limit. */
static char *utf8_argument(const wchar_t *argument) {
    size_t units = 0;
    while (units < 32767 && argument[units] != L'\0') ++units;
    if (units == 32767) return NULL;
    int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument,
        (int)(units + 1), NULL, 0, NULL, NULL);
    if (count <= 0 || count > 32767 * 3) return NULL;
    char *bytes = (char *)malloc((size_t)count);
    if (!bytes) return NULL;
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument,
            (int)(units + 1), bytes, count, NULL, NULL) != count) {
        free(bytes);
        return NULL;
    }
    return bytes;
}
int wmain(int argc, wchar_t **argv) {
    char program[] = "read_member";
    char *converted[3] = {program};
    if (argc != 3) return utf8_main(argc, converted);
    for (int index = 1; index < 3; ++index) {
        converted[index] = utf8_argument(argv[index]);
        if (!converted[index]) {
            fprintf(stderr, "invalid Unicode argument or argument allocation failure\n");
            for (int previous = 1; previous < index; ++previous) free(converted[previous]);
            return 1;
        }
    }
    int result = utf8_main(argc, converted);
    for (int index = 1; index < 3; ++index) free(converted[index]);
    return result;
}
#endif
