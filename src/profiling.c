#include "profiling.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define INITIAL_BLOCK_CAPACITY 1024
#define GROWTH_FACTOR 2

static double get_time_diff(const struct timespec* start, const struct timespec* end) {
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_nsec - start->tv_nsec) / 1e9;
}

ProfilingContext* profiling_create(void) {
    ProfilingContext* ctx = (ProfilingContext*)calloc(1, sizeof(ProfilingContext));
    if (!ctx) return NULL;
    
    ctx->block_profiles = (BlockProfile*)calloc(INITIAL_BLOCK_CAPACITY, 
                                              sizeof(BlockProfile));
    if (!ctx->block_profiles) {
        free(ctx);
        return NULL;
    }
    
    ctx->max_blocks = INITIAL_BLOCK_CAPACITY;
    ctx->num_blocks = 0;
    ctx->enabled = false;
    ctx->log_file = NULL;
    
    clock_gettime(CLOCK_MONOTONIC, &ctx->stats.start_time);
    return ctx;
}

void profiling_destroy(ProfilingContext* ctx) {
    if (!ctx) return;
    if (ctx->log_file) fclose(ctx->log_file);
    free(ctx->block_profiles);
    free(ctx);
}

void profiling_reset(ProfilingContext* ctx) {
    if (!ctx) return;
    memset(&ctx->stats, 0, sizeof(ProfilingStats));
    memset(ctx->block_profiles, 0, ctx->max_blocks * sizeof(BlockProfile));
    ctx->num_blocks = 0;
    clock_gettime(CLOCK_MONOTONIC, &ctx->stats.start_time);
}

void profiling_enable(ProfilingContext* ctx) {
    if (ctx) ctx->enabled = true;
}

void profiling_disable(ProfilingContext* ctx) {
    if (ctx) ctx->enabled = false;
}

void profiling_set_log_file(ProfilingContext* ctx, const char* filename) {
    if (!ctx || !filename) return;
    
    if (ctx->log_file) {
        fclose(ctx->log_file);
    }
    
    ctx->log_file = fopen(filename, "w");
}

void profiling_record_event(ProfilingContext* ctx, ProfilingEventType event) {
    if (!ctx || !ctx->enabled) return;
    
    switch (event) {
        case PROF_INSTRUCTION_EXECUTED:
            ctx->stats.total_instructions++;
            break;
        case PROF_BLOCK_COMPILED:
            ctx->stats.total_blocks_compiled++;
            break;
        case PROF_BLOCK_EXECUTED:
            ctx->stats.total_blocks_executed++;
            break;
        case PROF_CACHE_HIT:
            ctx->stats.cache_hits++;
            break;
        case PROF_CACHE_MISS:
            ctx->stats.cache_misses++;
            break;
        case PROF_MEMORY_READ:
            ctx->stats.memory_reads++;
            break;
        case PROF_MEMORY_WRITE:
            ctx->stats.memory_writes++;
            break;
        case PROF_BRANCH_TAKEN:
            ctx->stats.branches_taken++;
            break;
        case PROF_BRANCH_NOT_TAKEN:
            ctx->stats.branches_not_taken++;
            break;
    }
}

static BlockProfile* find_or_create_block_profile(ProfilingContext* ctx, uint64_t address) {
    // Search for existing block
    for (size_t i = 0; i < ctx->num_blocks; i++) {
        if (ctx->block_profiles[i].address == address) {
            return &ctx->block_profiles[i];
        }
    }
    
    // Create new block if space available
    if (ctx->num_blocks >= ctx->max_blocks) {
        size_t new_capacity = ctx->max_blocks * GROWTH_FACTOR;
        BlockProfile* new_profiles = (BlockProfile*)realloc(ctx->block_profiles,
                                                          new_capacity * sizeof(BlockProfile));
        if (!new_profiles) return NULL;
        
        ctx->block_profiles = new_profiles;
        ctx->max_blocks = new_capacity;
    }
    
    BlockProfile* profile = &ctx->block_profiles[ctx->num_blocks++];
    profile->address = address;
    return profile;
}

void profiling_record_block_execution(ProfilingContext* ctx, uint64_t address,
                                    uint64_t instruction_count, double execution_time) {
    if (!ctx || !ctx->enabled) return;
    
    BlockProfile* profile = find_or_create_block_profile(ctx, address);
    if (!profile) return;
    
    profile->execution_count++;
    profile->total_time += execution_time;
    profile->avg_time = profile->total_time / profile->execution_count;
    profile->instruction_count = instruction_count;
}

BlockProfile* profiling_get_block_stats(ProfilingContext* ctx, uint64_t address) {
    if (!ctx) return NULL;
    
    for (size_t i = 0; i < ctx->num_blocks; i++) {
        if (ctx->block_profiles[i].address == address) {
            return &ctx->block_profiles[i];
        }
    }
    return NULL;
}

void profiling_print_stats(const ProfilingContext* ctx) {
    if (!ctx) return;
    
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    double total_time = get_time_diff(&ctx->stats.start_time, &current_time);
    
    printf("\nProfiling Statistics:\n");
    printf("Total Time: %.3f seconds\n", total_time);
    printf("Instructions Executed: %lu\n", ctx->stats.total_instructions);
    printf("Blocks Compiled: %lu\n", ctx->stats.total_blocks_compiled);
    printf("Blocks Executed: %lu\n", ctx->stats.total_blocks_executed);
    printf("Cache Hit Rate: %.2f%%\n", profiling_get_cache_hit_rate(ctx) * 100.0);
    printf("IPC: %.2f\n", profiling_get_ipc(ctx));
    printf("Memory Operations: %lu reads, %lu writes\n",
           ctx->stats.memory_reads, ctx->stats.memory_writes);
    printf("Branch Statistics: %lu taken, %lu not taken\n",
           ctx->stats.branches_taken, ctx->stats.branches_not_taken);
}

void profiling_export_json(const ProfilingContext* ctx, const char* filename) {
    if (!ctx || !filename) return;
    
    FILE* file = fopen(filename, "w");
    if (!file) return;
    
    fprintf(file, "{\n");
    fprintf(file, "  \"stats\": {\n");
    fprintf(file, "    \"total_instructions\": %lu,\n", ctx->stats.total_instructions);
    fprintf(file, "    \"total_blocks_compiled\": %lu,\n", ctx->stats.total_blocks_compiled);
    fprintf(file, "    \"total_blocks_executed\": %lu,\n", ctx->stats.total_blocks_executed);
    fprintf(file, "    \"cache_hits\": %lu,\n", ctx->stats.cache_hits);
    fprintf(file, "    \"cache_misses\": %lu,\n", ctx->stats.cache_misses);
    fprintf(file, "    \"memory_reads\": %lu,\n", ctx->stats.memory_reads);
    fprintf(file, "    \"memory_writes\": %lu,\n", ctx->stats.memory_writes);
    fprintf(file, "    \"branches_taken\": %lu,\n", ctx->stats.branches_taken);
    fprintf(file, "    \"branches_not_taken\": %lu\n", ctx->stats.branches_not_taken);
    fprintf(file, "  },\n");
    
    fprintf(file, "  \"blocks\": [\n");
    for (size_t i = 0; i < ctx->num_blocks; i++) {
        const BlockProfile* profile = &ctx->block_profiles[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"address\": \"0x%lx\",\n", profile->address);
        fprintf(file, "      \"execution_count\": %lu,\n", profile->execution_count);
        fprintf(file, "      \"total_time\": %.9f,\n", profile->total_time);
        fprintf(file, "      \"avg_time\": %.9f,\n", profile->avg_time);
        fprintf(file, "      \"instruction_count\": %lu\n", profile->instruction_count);
        fprintf(file, "    }%s\n", i < ctx->num_blocks - 1 ? "," : "");
    }
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
}

double profiling_get_ipc(const ProfilingContext* ctx) {
    if (!ctx || ctx->stats.total_execution_time <= 0) return 0.0;
    return (double)ctx->stats.total_instructions / ctx->stats.total_execution_time;
}

double profiling_get_cache_hit_rate(const ProfilingContext* ctx) {
    if (!ctx) return 0.0;
    uint64_t total = ctx->stats.cache_hits + ctx->stats.cache_misses;
    return total > 0 ? (double)ctx->stats.cache_hits / total : 0.0;
}

void profiling_identify_hot_blocks(const ProfilingContext* ctx,
                                 uint64_t** hot_blocks, size_t* num_hot_blocks,
                                 double threshold) {
    if (!ctx || !hot_blocks || !num_hot_blocks) return;
    
    // Calculate total execution count
    uint64_t total_executions = 0;
    for (size_t i = 0; i < ctx->num_blocks; i++) {
        total_executions += ctx->block_profiles[i].execution_count;
    }
    
    // Count hot blocks
    size_t count = 0;
    for (size_t i = 0; i < ctx->num_blocks; i++) {
        double ratio = (double)ctx->block_profiles[i].execution_count / total_executions;
        if (ratio >= threshold) count++;
    }
    
    // Allocate and fill hot blocks array
    *hot_blocks = (uint64_t*)malloc(count * sizeof(uint64_t));
    if (!*hot_blocks) {
        *num_hot_blocks = 0;
        return;
    }
    
    size_t index = 0;
    for (size_t i = 0; i < ctx->num_blocks; i++) {
        double ratio = (double)ctx->block_profiles[i].execution_count / total_executions;
        if (ratio >= threshold) {
            (*hot_blocks)[index++] = ctx->block_profiles[i].address;
        }
    }
    
    *num_hot_blocks = count;
}

void profiling_log_message(ProfilingContext* ctx, const char* format, ...) {
    if (!ctx || !ctx->enabled || !ctx->log_file || !format) return;
    
    va_list args;
    va_start(args, format);
    vfprintf(ctx->log_file, format, args);
    fprintf(ctx->log_file, "\n");
    fflush(ctx->log_file);
    va_end(args);
}

void profiling_dump_block_info(const ProfilingContext* ctx, uint64_t address) {
    if (!ctx) return;
    
    const BlockProfile* profile = profiling_get_block_stats(ctx, address);
    if (!profile) {
        printf("No profile information for block at 0x%lx\n", address);
        return;
    }
    
    printf("Block Profile for 0x%lx:\n", address);
    printf("  Execution Count: %lu\n", profile->execution_count);
    printf("  Total Time: %.9f seconds\n", profile->total_time);
    printf("  Average Time: %.9f seconds\n", profile->avg_time);
    printf("  Instructions: %lu\n", profile->instruction_count);
    printf("  IPC: %.2f\n", profile->instruction_count / profile->avg_time);
} 