/* Bounded runtime asset access through the existing C ABI (also C++17).
 * Build shared C: cc -std=c11 -O2 -Wall -Wextra -Werror -I SDK/include
 *   runtime_asset_reader.c -L SDK/linux-x86_64 -lgradient_data_c -o reader
 * Build static C++: c++ -x c++ -std=c++17 -DDGRAD_STATIC -I SDK/include
 *   runtime_asset_reader.c SDK/linux-x86_64/libgradient_data_c.a
 *   -lpthread -ldl -lm -o reader-static
 * Native MSVC: cl /W4 /WX /std:c11 /TC /MD /I SDK/include
 *   runtime_asset_reader.c SDK/windows-x86_64/gradient_data_c.dll.lib
 * Use /TP /std:c++17 /MT /DDGRAD_STATIC with gradient_data_c.lib for C++
 * and link advapi32.lib userenv.lib ws2_32.lib bcrypt.lib ntdll.lib
 * kernel32.lib synchronization.lib (the same imports as the SDK CMake target).
 *
 * reader archive ARCHIVE.uacd MEMBER [OFFSET LENGTH]
 * reader loose FILE [OFFSET LENGTH]
 * With no range, read first/middle/tail windows of at most 4096 bytes each.
 * An explicit range is <=65536 bytes; even an empty file is read with length 0.
 * The archive handle closes BEFORE the first read; this application retains its
 * own member handle until all reads finish. No unpack or full verify is called.
 * read_at verifies touched codec blocks, whose work can exceed requested bytes.
 * Each mode performs the same bounded window reads and SHA-256 computation.
 * Timings describe one synchronous process: open includes metadata/member open,
 * info and parent close; read excludes hashing; total includes hashes and cleanup.
 * Cache state is unmanaged. Repeating a process is not proof of a warm/cold cache.
 * OS I/O counters cover the whole open/read/close interval, are process-wide,
 * and do not prove physical disk bytes or exclusive access to requested bytes.
 * getrusage_ru_inblock counts filesystem input operations, in OS-defined units;
 * Windows ReadTransferCount is an OS process transfer count in bytes, not a
 * physical-disk counter. Cached reads can contribute zero block-input operations.
 * No asset contents, filenames or SDK error text are emitted (errors may contain
 * private paths). Inputs must remain unchanged during use. This is an example,
 * not a mount, game integration, benchmark ranking or latency guarantee.
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#endif
#include "uacd_archive.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#define WINDOW_MAX 65536u
#define DEFAULT_WINDOW 4096u

typedef struct window_result {
    uint64_t offset;
    size_t length, written;
    int32_t status;
    double milliseconds;
    char sha256[65];
} window_result;

/* A small, independent SHA-256 implementation for result comparison only.
 * It never replaces SDK block verification or authenticates an archive author. */
static uint32_t rotate(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
static void sha_block(uint32_t h[8], const uint8_t *bytes) {
    static const uint32_t k[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    uint32_t w[64], a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], v=h[7];
    unsigned i;
    for (i=0; i<16; ++i) w[i]=((uint32_t)bytes[4*i]<<24)|((uint32_t)bytes[4*i+1]<<16)|((uint32_t)bytes[4*i+2]<<8)|bytes[4*i+3];
    for (i=16; i<64; ++i) {
        uint32_t s0=rotate(w[i-15],7)^rotate(w[i-15],18)^(w[i-15]>>3);
        uint32_t s1=rotate(w[i-2],17)^rotate(w[i-2],19)^(w[i-2]>>10);
        w[i]=w[i-16]+s0+w[i-7]+s1;
    }
    for (i=0; i<64; ++i) {
        uint32_t t1=v+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+k[i]+w[i];
        uint32_t t2=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));
        v=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=v;
}
static void sha256(const uint8_t *bytes, size_t length, char output[65]) {
    uint32_t h[8]={0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    uint8_t tail[128]={0};
    static const char hex[]="0123456789abcdef";
    size_t consumed=0, remaining, padded, i;
    uint64_t bits=(uint64_t)length*8;
    while (length-consumed>=64) { sha_block(h,bytes+consumed); consumed+=64; }
    remaining=length-consumed;
    if (remaining) memcpy(tail,bytes+consumed,remaining);
    tail[remaining]=0x80;
    padded=remaining<56 ? 64 : 128;
    for (i=0; i<8; ++i) tail[padded-1-i]=(uint8_t)(bits>>(8*i));
    sha_block(h,tail);
    if (padded==128) sha_block(h,tail+64);
    for (i=0; i<32; ++i) {
        uint8_t byte=(uint8_t)(h[i/4]>>(24-8*(i%4)));
        output[2*i]=hex[byte>>4]; output[2*i+1]=hex[byte&15];
    }
    output[64]='\0';
}
static int number(const char *text, uint64_t *value) {
    uint64_t n=0;
    if (!*text) return 0;
    while (*text) {
        unsigned digit=(unsigned)(unsigned char)*text++ - (unsigned)'0';
        if (digit>9 || n>(UINT64_MAX-digit)/10) return 0;
        n=n*10+digit;
    }
    *value=n; return 1;
}
static double now_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER count, frequency;
    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&count)) return -1;
    return (double)count.QuadPart*1000.0/(double)frequency.QuadPart;
#else
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC,&value)!=0) return -1;
    return (double)value.tv_sec*1000.0+(double)value.tv_nsec/1000000.0;
#endif
}
#ifdef _WIN32
typedef HANDLE loose_handle;
#define NO_LOOSE INVALID_HANDLE_VALUE
typedef struct io_sample { int valid; IO_COUNTERS value; } io_sample;
static io_sample sample_io(void) {
    io_sample sample;
    memset(&sample,0,sizeof sample);
    sample.valid=GetProcessIoCounters(GetCurrentProcess(),&sample.value)!=0;
    return sample;
}
/* SDK paths and the loose-file control both use strict UTF-8. */
static wchar_t *wide_path(const char *path) {
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);
    wchar_t *wide;
    if (count<=0 || count>32767) return NULL;
    wide=(wchar_t *)malloc((size_t)count*sizeof(*wide));
    if (!wide) return NULL;
    if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,count)!=count) { free(wide); return NULL; }
    return wide;
}
static int32_t loose_open(const char *path, loose_handle *handle, uint64_t *size) {
    wchar_t *wide=wide_path(path);
    LARGE_INTEGER bytes;
    if (!wide) return DGRAD_INVALID_ARGUMENT;
    *handle=CreateFileW(wide,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    free(wide);
    if (*handle==NO_LOOSE) return DGRAD_IO;
    if (GetFileType(*handle)!=FILE_TYPE_DISK || !GetFileSizeEx(*handle,&bytes) || bytes.QuadPart<0) return DGRAD_IO;
    *size=(uint64_t)bytes.QuadPart; return DGRAD_OK;
}
static int32_t loose_read(loose_handle handle, uint64_t offset, size_t length, uint8_t *bytes, size_t *written) {
    LARGE_INTEGER position;
    DWORD count=0;
    if (offset>INT64_MAX) return DGRAD_INVALID_ARGUMENT;
    position.QuadPart=(LONGLONG)offset;
    if (!SetFilePointerEx(handle,position,NULL,FILE_BEGIN) || !ReadFile(handle,bytes,(DWORD)length,&count,NULL)) return DGRAD_IO;
    *written=(size_t)count; return *written==length ? DGRAD_OK : DGRAD_IO;
}
static int32_t loose_close(loose_handle handle) { return CloseHandle(handle) ? DGRAD_OK : DGRAD_IO; }
#else
typedef int loose_handle;
#define NO_LOOSE (-1)
typedef struct io_sample { int valid; struct rusage value; } io_sample;
static io_sample sample_io(void) {
    io_sample sample;
    memset(&sample,0,sizeof sample); sample.valid=getrusage(RUSAGE_SELF,&sample.value)==0; return sample;
}
static int32_t loose_open(const char *path, loose_handle *handle, uint64_t *size) {
    struct stat info;
    *handle=open(path,O_RDONLY);
    if (*handle<0) return DGRAD_IO;
    if (fstat(*handle,&info)!=0 || !S_ISREG(info.st_mode) || info.st_size<0) return DGRAD_IO;
    *size=(uint64_t)info.st_size; return DGRAD_OK;
}
static int32_t loose_read(loose_handle handle, uint64_t offset, size_t length, uint8_t *bytes, size_t *written) {
    if (offset>INT64_MAX) return DGRAD_INVALID_ARGUMENT;
    if (!length) return DGRAD_OK;
    while (*written<length) {
        ssize_t count=pread(handle,bytes+*written,length-*written,(off_t)(offset+*written));
        if (count<0 && errno==EINTR) continue;
        if (count<=0) return DGRAD_IO;
        *written+=(size_t)count;
    }
    return DGRAD_OK;
}
static int32_t loose_close(loose_handle handle) { return close(handle)==0 ? DGRAD_OK : DGRAD_IO; }
#endif
static void print_io(io_sample before, io_sample after) {
    if (!before.valid || !after.valid) { printf("null"); return; }
#ifdef _WIN32
    printf("{\"windows_process_read_operation_count\":%llu,\"windows_process_read_transfer_count\":%llu}",
        (unsigned long long)(after.value.ReadOperationCount-before.value.ReadOperationCount),
        (unsigned long long)(after.value.ReadTransferCount-before.value.ReadTransferCount));
#else
    printf("{\"getrusage_ru_inblock\":%llu}",
        (unsigned long long)(after.value.ru_inblock-before.value.ru_inblock));
#endif
}
static int usage(void) {
    fprintf(stderr,"usage: runtime_asset_reader archive ARCHIVE MEMBER [OFFSET LENGTH]\n"
                   "       runtime_asset_reader loose FILE [OFFSET LENGTH]\n");
    printf("{\"schema\":\"uacd-runtime-reader-v1\",\"status\":1,\"error_phase\":\"arguments\"}\n");
    return 2;
}
#ifdef _WIN32
static int utf8_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    int archive_mode, explicit_range, argument, i, count=0;
    uint64_t raw_bytes=0, offset=0, length=0;
    uint8_t buffer[WINDOW_MAX];
    window_result windows[3];
    uacd_archive archive=0;
    dgrad_reader reader=0;
    loose_handle loose=NO_LOOSE;
    dgrad_info info;
    dgrad_read_limits limits;
    int32_t status=DGRAD_OK, closed;
    const char *phase="none";
    double start, open_ms=0, close_start, close_ms=0, total_ms;
    io_sample before, after;
    memset(windows,0,sizeof windows);
    memset(&info,0,sizeof info);
    memset(&limits,0,sizeof limits);
    if (argc<3) return usage();
    if (strcmp(argv[1],"archive")==0) archive_mode=1;
    else if (strcmp(argv[1],"loose")==0) archive_mode=0;
    else return usage();
    argument=archive_mode ? 4 : 3;
    if (argc!=argument && argc!=argument+2) return usage();
    explicit_range=argc==argument+2;
    if (explicit_range && (!number(argv[argument],&offset) || !number(argv[argument+1],&length)
        || length>WINDOW_MAX || offset>UINT64_MAX-length)) return usage();
    if (archive_mode) {
        status=dgrad_read_limits_init(&limits,sizeof limits);
        if (status) return usage();
        /* Application admission policy, not an OS memory/time quota. */
        limits.max_raw_bytes=UINT64_C(64)*1024*1024*1024;
        limits.max_index_bytes=UINT64_C(64)*1024*1024;
        limits.max_archive_bytes=UINT64_C(128)*1024*1024*1024;
    }
    before=sample_io(); start=now_ms();
    if (start<0) { fprintf(stderr,"monotonic clock unavailable\n"); return 1; }
    phase="open";
    if (archive_mode) {
        status=uacd_archive_open_with_limits(argv[2],&limits,UACD_ARCHIVE_DEFAULT_MAX_ENTRIES,&archive);
        if (!status) { phase="member_open"; status=uacd_archive_member_open(archive,argv[3],&reader); }
        /* Record the first status before cleanup can replace TLS error text.
         * Only numeric status/phase is reported, so private error paths stay local. */
        if (archive) {
            closed=uacd_archive_close(archive); archive=0;
            if (!status && closed) { status=closed; phase="archive_close"; }
        }
        if (!status) { phase="info"; status=dgrad_reader_info(reader,&info,sizeof info); raw_bytes=info.raw_bytes; }
    } else status=loose_open(argv[2],&loose,&raw_bytes);
    open_ms=now_ms()-start;
    if (!status) {
        if (explicit_range) { windows[0].offset=offset; windows[0].length=(size_t)length; count=1; }
        else {
            size_t n=raw_bytes<DEFAULT_WINDOW ? (size_t)raw_bytes : DEFAULT_WINDOW;
            windows[0].length=n;
            windows[1].offset=(raw_bytes-n)/2; windows[1].length=n;
            windows[2].offset=raw_bytes-n; windows[2].length=n;
            count=3;
        }
        phase="bounds";
        /* Subtraction avoids wrapping offset+length. SDK errors never truncate. */
        for (i=0; i<count; ++i) {
            if (windows[i].offset>raw_bytes || windows[i].length>raw_bytes-windows[i].offset) {
                status=DGRAD_INVALID_ARGUMENT; windows[i].status=status; break;
            }
        }
        if (!status) for (i=0; i<count; ++i) {
            double read_start=now_ms();
            phase="read";
            windows[i].status=archive_mode ? dgrad_reader_read_at(reader,windows[i].offset,windows[i].length,
                buffer,sizeof buffer,&windows[i].written) : loose_read(loose,windows[i].offset,windows[i].length,buffer,&windows[i].written);
            windows[i].milliseconds=now_ms()-read_start;
            status=windows[i].status;
            if (!status && windows[i].written!=windows[i].length) status=windows[i].status=DGRAD_IO;
            if (status) { count=i+1; break; }
            sha256(buffer,windows[i].written,windows[i].sha256);
        }
    }
    close_start=now_ms();
    if (reader) { closed=dgrad_reader_close(reader); if (!status && closed) { status=closed; phase="reader_close"; } }
    if (loose!=NO_LOOSE) { closed=loose_close(loose); if (!status && closed) { status=closed; phase="file_close"; } }
    close_ms=now_ms()-close_start;
    total_ms=now_ms()-start; after=sample_io();
    if (!status) phase="none";
    printf("{\"schema\":\"uacd-runtime-reader-v1\",\"mode\":\"%s\",\"status\":%d,\"error_phase\":\"%s\","
           "\"logical_bytes\":%llu,\"cache_state\":\"unmanaged\",\"full_verify_called\":false,\"unpack_called\":false,"
           "\"open_ms\":%.6f,\"close_ms\":%.6f,\"total_ms_including_hashes\":%.6f,\"windows\":[",
           archive_mode ? "archive" : "loose",(int)status,phase,(unsigned long long)raw_bytes,open_ms,close_ms,total_ms);
    for (i=0; i<count; ++i) {
        printf("%s{\"offset\":%llu,\"requested_bytes\":%llu,\"returned_bytes\":%llu,\"status\":%d,\"read_ms\":%.6f,\"sha256\":",
            i ? "," : "",(unsigned long long)windows[i].offset,(unsigned long long)windows[i].length,
            (unsigned long long)windows[i].written,(int)windows[i].status,windows[i].milliseconds);
        if (windows[i].sha256[0]) printf("\"%s\"}",windows[i].sha256); else printf("null}");
    }
    printf("],\"os_io_scope\":\"process_open_read_hash_close\",\"os_io_counters\":");
    print_io(before,after); printf("}\n");
    if (fflush(stdout)!=0) return 1;
    return status ? 1 : 0;
}
#ifdef _WIN32
/* Same bounded UTF-16 -> strict UTF-8 entry convention as read_member.c. */
static char *utf8_argument(const wchar_t *argument) {
    size_t units=0;
    int count;
    char *bytes;
    while (units<32767 && argument[units]!=L'\0') ++units;
    if (units==32767) return NULL;
    count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argument,(int)(units+1),NULL,0,NULL,NULL);
    if (count<=0 || count>32767*3) return NULL;
    bytes=(char *)malloc((size_t)count);
    if (!bytes) return NULL;
    if (WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argument,(int)(units+1),bytes,count,NULL,NULL)!=count) { free(bytes); return NULL; }
    return bytes;
}
int wmain(int argc, wchar_t **argv) {
    char program[]="runtime_asset_reader";
    char *converted[7]={program};
    int index, result;
    if (argc<3 || argc>6) return usage();
    for (index=1; index<argc; ++index) {
        converted[index]=utf8_argument(argv[index]);
        if (!converted[index]) {
            fprintf(stderr,"invalid Unicode argument or argument allocation failure\n");
            for (int previous=1; previous<index; ++previous) free(converted[previous]);
            return 1;
        }
    }
    result=utf8_main(argc,converted);
    for (index=1; index<argc; ++index) free(converted[index]);
    return result;
}
#endif
