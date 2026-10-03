#ifndef GRADIENT_DATA_HPP_INCLUDED
#define GRADIENT_DATA_HPP_INCLUDED
#include "gradient_data.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <limits>
namespace gradient_data {
class error : public std::runtime_error {
    int32_t status_;
public:
    error(int32_t status,const std::string& message) : std::runtime_error(message),status_(status) {}
    int32_t status() const noexcept { return status_; }
};
inline void check(int32_t status) {
    if (status == DGRAD_OK) return;
    const size_t n=dgrad_last_error(nullptr,0);
    std::vector<char> message(n ? n : 1);
    dgrad_last_error(message.data(),message.size());
    throw error(status,std::string("Gradient Data (")+std::to_string(status)+"): "+message.data());
}
/* Move-only ownership; no C++ exception crosses into a Rust callback. */
class reader {
    dgrad_reader handle_=0;
public:
    explicit reader(const std::string& utf8_path) {
        validate_path(utf8_path);
        check(dgrad_reader_open(utf8_path.c_str(),&handle_));
    }
    reader(const std::string& utf8_path,const dgrad_read_limits& limits) {
        validate_path(utf8_path);
        check(dgrad_reader_open_with_limits(utf8_path.c_str(),&limits,&handle_));
    }
    static void validate_path(const std::string& p) {
        if(p.empty() || p.find('\0')!=std::string::npos)
            throw std::invalid_argument("Gradient Data path must be nonempty and contain no embedded NUL");
    }
    ~reader() noexcept { if(handle_) (void)dgrad_reader_close(handle_); }
    reader(const reader&)=delete;
    reader& operator=(const reader&)=delete;
    reader(reader&& r) noexcept : handle_(std::exchange(r.handle_,0)) {}
    reader& operator=(reader&& r) noexcept {
        if(this!=&r){if(handle_) (void)dgrad_reader_close(handle_);handle_=std::exchange(r.handle_,0);}return *this;
    }
    dgrad_info info() const {dgrad_info value{};check(dgrad_reader_info(handle_,&value,sizeof value));return value;}
    void verify() const {check(dgrad_reader_verify(handle_));}
    std::vector<uint8_t> read(uint64_t offset,size_t length,
        size_t max_request_bytes=std::numeric_limits<size_t>::max()) const {
        if(length>max_request_bytes) throw error(DGRAD_LIMIT,"Gradient Data request allocation exceeds caller budget");
        const auto i=info();
        if(offset>i.raw_bytes || length>i.raw_bytes-offset) throw std::out_of_range("Gradient Data range outside file");
        std::vector<uint8_t> out(length);size_t written=0;
        check(dgrad_reader_read_at(handle_,offset,length,out.data(),out.size(),&written));
        if(written!=length) throw std::runtime_error("Gradient Data short successful read");
        return out;
    }
};
}
#endif
