#pragma once
#include <AK/Types.h>

struct CpioHeader {
    char c_magic[6];    // "070701"
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
};

class CpioFileSystem {
public:
    static void initialize(uint8_t* address);
    static CpioFileSystem& the();

    CpioFileSystem(uint8_t* address);
    uint8_t* find_file(const char* path, uint32_t* out_size) const;

private:
    uint8_t* m_address { nullptr };
};