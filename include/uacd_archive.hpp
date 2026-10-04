#ifndef UACD_ARCHIVE_HPP_INCLUDED
#define UACD_ARCHIVE_HPP_INCLUDED
#include "uacd_archive.h"
#include "gradient_data.hpp"
namespace gradient_data {
class member_reader {
    dgrad_reader handle_=0;
public:
    member_reader(uacd_archive archive,const std::string& member) {
        reader::validate_path(member);
        check(uacd_archive_member_open(archive,member.c_str(),&handle_));
    }
    ~member_reader() noexcept {if(handle_) (void)dgrad_reader_close(handle_);}
    member_reader(const member_reader&)=delete;
    member_reader& operator=(const member_reader&)=delete;
    member_reader(member_reader&& other) noexcept : handle_(std::exchange(other.handle_,0)) {}
    member_reader& operator=(member_reader&& other) noexcept {
        if(this!=&other) {if(handle_) (void)dgrad_reader_close(handle_); handle_=std::exchange(other.handle_,0);}return *this;
    }
    dgrad_info info() const {dgrad_info i{};check(dgrad_reader_info(handle_,&i,sizeof i));return i;}
    void verify() const {check(dgrad_reader_verify(handle_));}
    std::vector<uint8_t> read(uint64_t offset,size_t length,size_t maximum=std::numeric_limits<size_t>::max()) const {
        if(length>maximum) throw error(DGRAD_LIMIT,"member read exceeds caller budget");
        const auto i=info();
        if(offset>i.raw_bytes || length>i.raw_bytes-offset) throw std::out_of_range("member range outside file");
        std::vector<uint8_t> bytes(length);size_t written=0;
        check(dgrad_reader_read_at(handle_,offset,length,bytes.data(),bytes.size(),&written));
        if(written!=length) throw std::runtime_error("short member read");
        return bytes;
    }
};
class archive_reader {
    uacd_archive handle_=0;
public:
    explicit archive_reader(const std::string& path,uint64_t max_entries=UACD_ARCHIVE_DEFAULT_MAX_ENTRIES) {
        reader::validate_path(path);
        check(uacd_archive_open_with_limits(path.c_str(),nullptr,max_entries,&handle_));
    }
    archive_reader(const std::string& path,const dgrad_read_limits& limits,uint64_t max_entries) {
        reader::validate_path(path);
        check(uacd_archive_open_with_limits(path.c_str(),&limits,max_entries,&handle_));
    }
    ~archive_reader() noexcept {if(handle_) (void)uacd_archive_close(handle_);}
    archive_reader(const archive_reader&)=delete;
    archive_reader& operator=(const archive_reader&)=delete;
    archive_reader(archive_reader&& other) noexcept : handle_(std::exchange(other.handle_,0)) {}
    archive_reader& operator=(archive_reader&& other) noexcept {
        if(this!=&other) {if(handle_) (void)uacd_archive_close(handle_);handle_=std::exchange(other.handle_,0);}return *this;
    }
    member_reader member(const std::string& name) const {return member_reader(handle_,name);}
    void verify() const {check(uacd_archive_verify(handle_));}
    void extract(const std::string& name,const std::string& output) const {
        reader::validate_path(name);reader::validate_path(output);check(uacd_archive_extract(handle_,name.c_str(),output.c_str()));
    }
};
}
#endif
