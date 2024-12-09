#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    PERM_NONE = 0,
    PERM_READ = 1,
    PERM_WRITE = 2,
    PERM_EXEC = 4
} MemoryPermissions;

typedef struct MemoryRegion {
    uint64_t start;
    uint64_t size;
    uint8_t* data;
    MemoryPermissions permissions;
    struct MemoryRegion* next;
} MemoryRegion;

typedef struct Memory {
    MemoryRegion* regions;
    size_t total_mapped_size;
    bool little_endian;
} Memory;

Memory* memory_create(void);
void memory_destroy(Memory* mem);

bool memory_map(Memory* mem, uint64_t address, size_t size, MemoryPermissions perms);
bool memory_unmap(Memory* mem, uint64_t address, size_t size);
bool memory_protect(Memory* mem, uint64_t address, size_t size, MemoryPermissions perms);

bool memory_read8(Memory* mem, uint64_t address, uint8_t* value);
bool memory_read16(Memory* mem, uint64_t address, uint16_t* value);
bool memory_read32(Memory* mem, uint64_t address, uint32_t* value);
bool memory_read64(Memory* mem, uint64_t address, uint64_t* value);

bool memory_write8(Memory* mem, uint64_t address, uint8_t value);
bool memory_write16(Memory* mem, uint64_t address, uint16_t value);
bool memory_write32(Memory* mem, uint64_t address, uint32_t value);
bool memory_write64(Memory* mem, uint64_t address, uint64_t value);

MemoryRegion* memory_find_region(Memory* mem, uint64_t address);
bool memory_is_mapped(Memory* mem, uint64_t address, size_t size);
bool memory_copy_to(Memory* mem, uint64_t address, const void* data, size_t size);
bool memory_copy_from(Memory* mem, uint64_t address, void* data, size_t size);

size_t memory_get_mapped_size(const Memory* mem);
void memory_print_regions(const Memory* mem);
bool memory_validate_access(const Memory* mem, uint64_t address, size_t size, MemoryPermissions required_perms);

#endif // MEMORY_H 