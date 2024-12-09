#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Memory* memory_create(void) {
    Memory* mem = (Memory*)calloc(1, sizeof(Memory));
    if (mem) {
        mem->regions = NULL;
        mem->total_mapped_size = 0;
        mem->little_endian = true;
    }
    return mem;
}

void memory_destroy(Memory* mem) {
    if (!mem) return;
    
    MemoryRegion* current = mem->regions;
    while (current) {
        MemoryRegion* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    free(mem);
}

static MemoryRegion* create_region(uint64_t start, size_t size, MemoryPermissions perms) {
    MemoryRegion* region = (MemoryRegion*)calloc(1, sizeof(MemoryRegion));
    if (!region) return NULL;
    
    region->data = (uint8_t*)calloc(1, size);
    if (!region->data) {
        free(region);
        return NULL;
    }
    
    region->start = start;
    region->size = size;
    region->permissions = perms;
    region->next = NULL;
    return region;
}

bool memory_map(Memory* mem, uint64_t address, size_t size, MemoryPermissions perms) {
    if (!mem || !size) return false;
    if (memory_is_mapped(mem, address, size)) return false;
    
    MemoryRegion* new_region = create_region(address, size, perms);
    if (!new_region) return false;
    
    if (!mem->regions || mem->regions->start > address) {
        new_region->next = mem->regions;
        mem->regions = new_region;
    } else {
        MemoryRegion* current = mem->regions;
        while (current->next && current->next->start < address) {
            current = current->next;
        }
        new_region->next = current->next;
        current->next = new_region;
    }
    
    mem->total_mapped_size += size;
    return true;
}

bool memory_unmap(Memory* mem, uint64_t address, size_t size) {
    if (!mem || !size) return false;
    
    MemoryRegion* current = mem->regions;
    MemoryRegion* prev = NULL;
    
    while (current) {
        if (current->start == address && current->size == size) {
            if (prev) {
                prev->next = current->next;
            } else {
                mem->regions = current->next;
            }
            mem->total_mapped_size -= size;
            free(current->data);
            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

MemoryRegion* memory_find_region(Memory* mem, uint64_t address) {
    if (!mem) return NULL;
    
    MemoryRegion* current = mem->regions;
    while (current) {
        if (address >= current->start && address < (current->start + current->size)) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

bool memory_protect(Memory* mem, uint64_t address, size_t size, MemoryPermissions perms) {
    MemoryRegion* region = memory_find_region(mem, address);
    if (!region || (address + size) > (region->start + region->size)) {
        return false;
    }
    region->permissions = perms;
    return true;
}

static bool check_access(const MemoryRegion* region, MemoryPermissions required_perms) {
    return (region->permissions & required_perms) == required_perms;
}

bool memory_read8(Memory* mem, uint64_t address, uint8_t* value) {
    MemoryRegion* region = memory_find_region(mem, address);
    if (!region || !check_access(region, PERM_READ)) return false;
    
    size_t offset = address - region->start;
    *value = region->data[offset];
    return true;
}

bool memory_write8(Memory* mem, uint64_t address, uint8_t value) {
    MemoryRegion* region = memory_find_region(mem, address);
    if (!region || !check_access(region, PERM_WRITE)) return false;
    
    size_t offset = address - region->start;
    region->data[offset] = value;
    return true;
}

bool memory_read16(Memory* mem, uint64_t address, uint16_t* value) {
    uint8_t bytes[2];
    for (int i = 0; i < 2; i++) {
        if (!memory_read8(mem, address + i, &bytes[i])) return false;
    }
    
    if (mem->little_endian) {
        *value = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    } else {
        *value = (uint16_t)bytes[1] | ((uint16_t)bytes[0] << 8);
    }
    return true;
}

bool memory_write16(Memory* mem, uint64_t address, uint16_t value) {
    uint8_t bytes[2];
    if (mem->little_endian) {
        bytes[0] = value & 0xFF;
        bytes[1] = (value >> 8) & 0xFF;
    } else {
        bytes[0] = (value >> 8) & 0xFF;
        bytes[1] = value & 0xFF;
    }
    
    for (int i = 0; i < 2; i++) {
        if (!memory_write8(mem, address + i, bytes[i])) return false;
    }
    return true;
}

bool memory_read32(Memory* mem, uint64_t address, uint32_t* value) {
    uint16_t words[2];
    for (int i = 0; i < 2; i++) {
        if (!memory_read16(mem, address + i * 2, &words[i])) return false;
    }
    
    if (mem->little_endian) {
        *value = (uint32_t)words[0] | ((uint32_t)words[1] << 16);
    } else {
        *value = (uint32_t)words[1] | ((uint32_t)words[0] << 16);
    }
    return true;
}

bool memory_write32(Memory* mem, uint64_t address, uint32_t value) {
    uint16_t words[2];
    if (mem->little_endian) {
        words[0] = value & 0xFFFF;
        words[1] = (value >> 16) & 0xFFFF;
    } else {
        words[0] = (value >> 16) & 0xFFFF;
        words[1] = value & 0xFFFF;
    }
    
    for (int i = 0; i < 2; i++) {
        if (!memory_write16(mem, address + i * 2, words[i])) return false;
    }
    return true;
}

bool memory_read64(Memory* mem, uint64_t address, uint64_t* value) {
    uint32_t dwords[2];
    for (int i = 0; i < 2; i++) {
        if (!memory_read32(mem, address + i * 4, &dwords[i])) return false;
    }
    
    if (mem->little_endian) {
        *value = (uint64_t)dwords[0] | ((uint64_t)dwords[1] << 32);
    } else {
        *value = (uint64_t)dwords[1] | ((uint64_t)dwords[0] << 32);
    }
    return true;
}

bool memory_write64(Memory* mem, uint64_t address, uint64_t value) {
    uint32_t dwords[2];
    if (mem->little_endian) {
        dwords[0] = value & 0xFFFFFFFF;
        dwords[1] = (value >> 32) & 0xFFFFFFFF;
    } else {
        dwords[0] = (value >> 32) & 0xFFFFFFFF;
        dwords[1] = value & 0xFFFFFFFF;
    }
    
    for (int i = 0; i < 2; i++) {
        if (!memory_write32(mem, address + i * 4, dwords[i])) return false;
    }
    return true;
}

bool memory_copy_to(Memory* mem, uint64_t address, const void* data, size_t size) {
    const uint8_t* src = (const uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        if (!memory_write8(mem, address + i, src[i])) return false;
    }
    return true;
}

bool memory_copy_from(Memory* mem, uint64_t address, void* data, size_t size) {
    uint8_t* dst = (uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        if (!memory_read8(mem, address + i, &dst[i])) return false;
    }
    return true;
}

bool memory_is_mapped(Memory* mem, uint64_t address, size_t size) {
    if (!mem || !size) return false;
    
    MemoryRegion* current = mem->regions;
    while (current) {
        if (address >= current->start && (address + size) <= (current->start + current->size)) {
            return true;
        }
        current = current->next;
    }
    return false;
}

size_t memory_get_mapped_size(const Memory* mem) {
    return mem ? mem->total_mapped_size : 0;
}

void memory_print_regions(const Memory* mem) {
    if (!mem) return;
    
    printf("Memory Regions:\n");
    MemoryRegion* current = mem->regions;
    while (current) {
        printf("0x%016lx - 0x%016lx (%zu bytes) [%c%c%c]\n",
               current->start,
               current->start + current->size,
               current->size,
               (current->permissions & PERM_READ) ? 'R' : '-',
               (current->permissions & PERM_WRITE) ? 'W' : '-',
               (current->permissions & PERM_EXEC) ? 'X' : '-');
        current = current->next;
    }
}

bool memory_validate_access(const Memory* mem, uint64_t address, size_t size, MemoryPermissions required_perms) {
    if (!mem || !size) return false;
    
    MemoryRegion* region = memory_find_region((Memory*)mem, address);
    if (!region || (address + size) > (region->start + region->size)) {
        return false;
    }
    return check_access(region, required_perms);
} 