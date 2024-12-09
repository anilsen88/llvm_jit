#ifndef PROFILING_H
#define PROFILING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef enum {
    PROF_INSTRUCTION_EXECUTED,
    PROF_BLOCK_COMPILED,
    PROF_BLOCK_EXECUTED,
    PROF_CACHE_HIT,
    PROF_CACHE_MISS,
    PROF_MEMORY_READ,
    PROF_MEMORY_WRITE,
    PROF_BRANCH_TAKEN,
    PROF_BRANCH_NOT_TAKEN
} ProfilingEventType;

typedef struct {
    uint64_t total_instructions;
    uint64_t total_blocks_compiled;
    uint64_t total_blocks_executed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t memory_reads;
    uint64_t memory_writes;
    uint64_t branches_taken;
    uint64_t branches_not_taken;
    
    struct timespec start_time;
    double total_execution_time;
    double total_compilation_time;
} ProfilingStats;

typedef struct {
    uint64_t address;
    uint64_t execution_count;
    double total_time;
    double avg_time;
    uint64_t instruction_count;
} BlockProfile;

typedef struct {
    ProfilingStats stats;
    BlockProfile* block_profiles;
    size_t num_blocks;
    size_t max_blocks;
    bool enabled;
    FILE* log_file;
} ProfilingContext;

ProfilingContext* profiling_create(void);
void profiling_destroy(ProfilingContext* ctx);
void profiling_reset(ProfilingContext* ctx);

void profiling_enable(ProfilingContext* ctx);
void profiling_disable(ProfilingContext* ctx);
void profiling_set_log_file(ProfilingContext* ctx, const char* filename);

void profiling_record_event(ProfilingContext* ctx, ProfilingEventType event);
void profiling_start_block(ProfilingContext* ctx, uint64_t address);
void profiling_end_block(ProfilingContext* ctx, uint64_t address);

void profiling_record_block_execution(ProfilingContext* ctx, uint64_t address, 
                                    uint64_t instruction_count, double execution_time);
BlockProfile* profiling_get_block_stats(ProfilingContext* ctx, uint64_t address);

void profiling_print_stats(const ProfilingContext* ctx);
void profiling_export_json(const ProfilingContext* ctx, const char* filename);
double profiling_get_ipc(const ProfilingContext* ctx);
double profiling_get_cache_hit_rate(const ProfilingContext* ctx);

void profiling_identify_hot_blocks(const ProfilingContext* ctx, 
                                 uint64_t** hot_blocks, size_t* num_hot_blocks,
                                 double threshold);

void profiling_log_message(ProfilingContext* ctx, const char* format, ...);
void profiling_dump_block_info(const ProfilingContext* ctx, uint64_t address);

#endif // PROFILING_H 