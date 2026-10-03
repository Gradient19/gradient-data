#include "gradient_data.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
int main(int argc, char **argv) {
    if (argc != 2) { std::cerr << "usage: read_range_cpp FILE.uacd\n"; return 2; }
    try {
        gradient_data::reader reader{std::string(argv[1])};
        reader.verify();
        const auto info = reader.info();
        const auto count = static_cast<size_t>(std::min<uint64_t>(info.raw_bytes, 32));
        const auto bytes = reader.read(0, count);
        std::cout << "read " << bytes.size() << " verified bytes from "
                  << info.raw_bytes << "-byte file\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
