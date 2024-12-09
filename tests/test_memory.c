#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/memory.h"

static void test_memory_allocation() {
    MemoryManager* mm = memory_manager_create();
    assert(mm != NULL);
    assert(mm->code_buffer != NULL);
    assert(mm->code_size == 0);
    assert(mm->code_capacity > 0);
    memory_manager_destroy(mm);
}

static void test_code_buffer_expansion() {
    MemoryManager* mm = memory_manager_create();
    size_t initial_capacity = mm->code_capacity;
    
    uint8_t test_data[1024];
    memset(test_data, 0x90, sizeof(test_data));
    
    for (int i = 0; i < 10; i++) {
        bool success = memory_manager_append(mm, test_data, sizeof(test_data));
        assert(success);
    }
    
    assert(mm->code_capacity > initial_capacity);
    assert(mm->code_size == 10 * sizeof(test_data));
    
    memory_manager_destroy(mm);
}

static void test_memory_protection() {
    MemoryManager* mm = memory_manager_create();
    uint8_t test_code[] = {
        0x48, 0xC7, 0xC0, 0x2A, 0x00, 0x00, 0x00, 
        0xC3                                        
    };
    
    bool success = memory_manager_append(mm, test_code, sizeof(test_code));
    assert(success);
    
    void* executable_buffer = memory_manager_make_executable(mm);
    assert(executable_buffer != NULL);
    
    uint64_t (*func)(void) = (uint64_t (*)(void))executable_buffer;
    uint64_t result = func();
    assert(result == 42);
    
    memory_manager_destroy(mm);
}

static void test_memory_alignment() {
    MemoryManager* mm = memory_manager_create();
    
    assert(((uintptr_t)mm->code_buffer % 16) == 0);
    
    uint8_t single_byte = 0x90;
    memory_manager_append(mm, &single_byte, 1);
    
    void* aligned_ptr = memory_manager_allocate_aligned(mm, 64, 32);
    assert(aligned_ptr != NULL);
    assert(((uintptr_t)aligned_ptr % 32) == 0);
    
    memory_manager_destroy(mm);
}

static void test_memory_boundaries() {
    MemoryManager* mm = memory_manager_create();
    size_t initial_size = mm->code_size;
    
    bool success = memory_manager_append(mm, NULL, 10);
    assert(!success);
    assert(mm->code_size == initial_size);
    
    success = memory_manager_append(mm, (uint8_t*)"test", 0);
    assert(success);
    assert(mm->code_size == initial_size);
    
    void* ptr = memory_manager_allocate_aligned(mm, 0, 16);
    assert(ptr == NULL);
    
    memory_manager_destroy(mm);
}

static void test_memory_read_write() {
    MemoryManager* mm = memory_manager_create();
    
    uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    bool success = memory_manager_append(mm, test_data, sizeof(test_data));
    assert(success);
    
    assert(memcmp(mm->code_buffer, test_data, sizeof(test_data)) == 0);
    
    size_t offset = 4;
    assert(memcmp(mm->code_buffer + offset, test_data + offset, sizeof(test_data) - offset) == 0);
    
    memory_manager_destroy(mm);
}

static void test_memory_reallocation() {
    MemoryManager* mm = memory_manager_create();
    size_t initial_capacity = mm->code_capacity;
    
    for (size_t i = 0; i < 1000; i++) {
        uint8_t data = (uint8_t)i;
        bool success = memory_manager_append(mm, &data, 1);
        assert(success);
    }
    
    assert(mm->code_capacity > initial_capacity);
    assert(mm->code_size == 1000);
    
    for (size_t i = 0; i < 1000; i++) {
        assert(mm->code_buffer[i] == (uint8_t)i);
    }
    
    memory_manager_destroy(mm);
}

static void test_memory_large_allocations() {
    MemoryManager* mm = memory_manager_create();
    
    void* ptr = memory_manager_allocate_aligned(mm, SIZE_MAX - 1000, 16);
    assert(ptr == NULL);

    size_t large_size = 1024 * 1024; 
    uint8_t* large_buffer = malloc(large_size);
    assert(large_buffer != NULL);
    
    for (size_t i = 0; i < large_size; i++) {
        large_buffer[i] = (uint8_t)(i & 0xFF);
    }
    
    bool success = memory_manager_append(mm, large_buffer, large_size);
    assert(success);
    assert(mm->code_size == large_size);
    
    assert(memcmp(mm->code_buffer, large_buffer, large_size) == 0);
    
    free(large_buffer);
    memory_manager_destroy(mm);
}

int main() {
    printf("Running memory manager tests...\n");
    
    test_memory_allocation();
    test_code_buffer_expansion();
    test_memory_protection();
    test_memory_alignment();
    test_memory_boundaries();
    test_memory_read_write();
    test_memory_reallocation();
    test_memory_large_allocations();
    
    printf("All memory manager tests passed!\n");
    return 0;
} 