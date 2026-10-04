#include "uacd_archive.hpp"
#include <algorithm>
#include <iostream>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <fcntl.h>
#include <io.h>
#endif
/* Usage: read_member ARCHIVE.uacd MEMBER. Bounded verified member output;
 * the parent archive closes before the independent member reader is used. */
#ifdef _WIN32
static int utf8_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " ARCHIVE.uacd MEMBER\n";
        return 2;
    }
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
        std::cerr << "could not set binary stdout\n";
        return 1;
    }
#endif
    try {
        auto member = [&] {
            gradient_data::archive_reader archive(argv[1]);
            archive.verify();
            return archive.member(argv[2]);
        }();
        member.verify();
        const auto info = member.info();
        const auto length = static_cast<size_t>(std::min<uint64_t>(info.raw_bytes, 4096));
        const auto bytes = member.read(0, length, 4096);
        if (!bytes.empty()) {
            std::cout.write(reinterpret_cast<const char *>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
        }
        std::cout.flush();
        if (!std::cout) throw std::runtime_error("could not write member bytes");
    } catch (const std::exception &error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
    return 0;
}

#ifdef _WIN32
// Native Windows argv is UTF-16; reject invalid sequences while converting
// to the SDK's UTF-8 paths. The bound includes the terminal nul code unit.
static std::string utf8_argument(const wchar_t *argument) {
    size_t units = 0;
    while (units < 32767 && argument[units] != L'\0') ++units;
    if (units == 32767) throw std::runtime_error("argument exceeds Windows command-line bound");
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument,
        static_cast<int>(units + 1), nullptr, 0, nullptr, nullptr);
    if (count <= 0 || count > 32767 * 3) throw std::runtime_error("invalid Unicode command-line argument");
    std::string bytes(static_cast<size_t>(count), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument,
            static_cast<int>(units + 1), bytes.data(), count, nullptr, nullptr) != count) {
        throw std::runtime_error("could not convert Unicode command-line argument");
    }
    bytes.pop_back();
    return bytes;
}
int wmain(int argc, wchar_t **argv) {
    char program[] = "read_member";
    char *converted[3] = {program};
    if (argc != 3) return utf8_main(argc, converted);
    try {
        auto archive = utf8_argument(argv[1]);
        converted[1] = archive.data();
        auto member = utf8_argument(argv[2]);
        converted[2] = member.data();
        return utf8_main(argc, converted);
    } catch (const std::exception &error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
#endif
