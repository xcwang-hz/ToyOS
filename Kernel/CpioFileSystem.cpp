#include <Kernel/CpioFileSystem.h>
#include <Kernel/kprintf.h>
#include <Kernel/StdLib.h>

static CpioFileSystem* s_the = nullptr;
void CpioFileSystem::initialize(uint8_t* address)
{
    if (s_the) 
        return;
    s_the = new CpioFileSystem(address);
}

CpioFileSystem& CpioFileSystem::the()
{
    return *s_the;
}

CpioFileSystem::CpioFileSystem(uint8_t* address)
    : m_address(address)
{
    kprintf("CpioFileSystem initialized @ %p\n", m_address);
}

uint8_t* CpioFileSystem::find_file(const char* target_path, uint32_t* out_size) const
{
    if (!m_address) 
        return nullptr;

    uint8_t* current = m_address;
    while (true) {
        auto* header = (CpioHeader*)current;
        if (strncmp(header->c_magic, "070701", 6) != 0) {
            kprintf("CPIO Error: Bad magic.\n");            
            return nullptr;
        }

        int namesize = hexstrtol(header->c_namesize);
        int filesize = hexstrtol(header->c_filesize);
        char* filename = (char*)(current + sizeof(CpioHeader));
        if (strcmp(filename, "TRAILER!!!") == 0) 
            return nullptr;

        // Calculate alignment (CPIO Data is 4-byte aligned)
        uint32_t head_len = sizeof(CpioHeader) + namesize;
        if (head_len % 4 != 0) head_len += 4 - (head_len % 4);
        
        uint8_t* file_data = current + head_len;

        // [Key] Compare filenames
        if (strcmp(filename, target_path) == 0 || (strlen(filename) > strlen(target_path) && 
            strcmp(filename + strlen(filename) - strlen(target_path), target_path) == 0)) 
        {
            if (out_size) *out_size = filesize;
            return file_data;
        }

        // Jump to next file
        uint32_t next_offset = head_len + filesize;
        if (next_offset % 4 != 0) next_offset += 4 - (next_offset % 4);
        current += next_offset;
    }

    return nullptr; // Not found
}