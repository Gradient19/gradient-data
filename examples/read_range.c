#include "gradient_data.h"
#include <stdint.h>
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
#endif
#ifdef _WIN32
static int utf8_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    dgrad_reader reader = 0;
    dgrad_info info = {0};
    uint8_t bytes[32] = {0};
    size_t written = 0;
    int status;
    if (argc != 2) { fprintf(stderr, "usage: read_range_c FILE.uacd\n"); return 2; }
    status = dgrad_reader_open(argv[1], &reader);
    if (status == DGRAD_OK) status = dgrad_reader_info(reader, &info, sizeof info);
    if (status == DGRAD_OK) {
        size_t count = info.raw_bytes < sizeof bytes ? (size_t)info.raw_bytes : sizeof bytes;
        status = dgrad_reader_read_at(reader, 0, count, bytes, sizeof bytes, &written);
    }
    if (reader) (void)dgrad_reader_close(reader);
    if (status != DGRAD_OK) {
        char error[256] = {0};
        (void)dgrad_last_error(error, sizeof error);
        fprintf(stderr, "Gradient Data error %d: %s\n", status, error);
        return 1;
    }
    printf("read %zu verified bytes from %llu-byte file\n", written,
           (unsigned long long)info.raw_bytes);
    return 0;
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
    char program[] = "read_range";
    char *converted[2] = {program};
    if (argc != 2) return utf8_main(argc, converted);
    for (int index = 1; index < 2; ++index) {
        converted[index] = utf8_argument(argv[index]);
        if (!converted[index]) {
            fprintf(stderr, "invalid Unicode argument or argument allocation failure\n");
            for (int previous = 1; previous < index; ++previous) free(converted[previous]);
            return 1;
        }
    }
    int result = utf8_main(argc, converted);
    for (int index = 1; index < 2; ++index) free(converted[index]);
    return result;
}
#endif
