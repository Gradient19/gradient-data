#include "gradient_data.h"
#include <stdint.h>
#include <stdio.h>
int main(int argc, char **argv) {
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
