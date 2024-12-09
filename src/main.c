#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "jit.h"
#include "memory.h"
#include "registers.h"
#include "profiling.h"

static struct option long_options[] = {
    {"input",     required_argument, 0, 'i'},
    {"output",    required_argument, 0, 'o'},
    {"debug",     no_argument,       0, 'd'},
    {"profile",   no_argument,       0, 'p'},
    {"help",      no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

typedef struct {
    char* input_file;
    char* output_file;
    bool debug_mode;
    bool profile_mode;
} Config;

static void print_usage(const char* program_name);
static bool parse_arguments(int argc, char** argv, Config* config);
static bool load_binary(const char* filename, Memory* memory, uint64_t* entry_point);
static void cleanup(JITContext* jit, Memory* memory, RegisterFile* registers, ProfilingContext* profiling);

int main(int argc, char** argv) {
    Config config = {0};
    uint64_t entry_point = 0;

    if (!parse_arguments(argc, argv, &config)) {
        return EXIT_FAILURE;
    }

    Memory* memory = memory_create();
    RegisterFile* registers = registers_create();
    ProfilingContext* profiling = profiling_create();

    if (!memory || !registers || !profiling) {
        fprintf(stderr, "Failed to initialize emulator components\n");
        cleanup(NULL, memory, registers, profiling);
        return EXIT_FAILURE;
    }

    if (!load_binary(config.input_file, memory, &entry_point)) {
        fprintf(stderr, "Failed to load input file: %s\n", config.input_file);
        cleanup(NULL, memory, registers, profiling);
        return EXIT_FAILURE;
    }

    JITContext* jit = jit_create(memory, registers);
    if (!jit) {
        fprintf(stderr, "Failed to initialize JIT compiler\n");
        cleanup(NULL, memory, registers, profiling);
        return EXIT_FAILURE;
    }

    if (config.profile_mode) {
        profiling_enable(profiling);
        if (config.output_file) {
            profiling_set_log_file(profiling, config.output_file);
        }
    }

    registers_set_pc(registers, entry_point);

    while (true) {
        uint64_t pc = registers_get_pc(registers);
        void* block = jit_get_cached_block(jit, pc);

        if (!block) {
            block = jit_compile_block(jit, pc);
            if (!block) {
                fprintf(stderr, "Failed to compile block at 0x%lx\n", pc);
                break;
            }
        }

        if (config.debug_mode) {
            printf("Executing block at 0x%lx\n", pc);
            registers_print_state(registers);
        }

        if (!jit_execute_block(jit, block)) {
            fprintf(stderr, "Execution failed at 0x%lx\n", pc);
            break;
        }

        if (registers_get_pc(registers) == 0) {
            break;
        }
    }

    if (config.profile_mode) {
        profiling_print_stats(profiling);
        if (config.output_file) {
            profiling_export_json(profiling, config.output_file);
        }
    }

    cleanup(jit, memory, registers, profiling);
    free(config.input_file);
    free(config.output_file);

    return EXIT_SUCCESS;
}

static void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -i, --input=FILE    Input ARM64 binary file\n");
    printf("  -o, --output=FILE   Output file for profiling data\n");
    printf("  -d, --debug         Enable debug mode\n");
    printf("  -p, --profile       Enable profiling\n");
    printf("  -h, --help          Display this help message\n");
}

static bool parse_arguments(int argc, char** argv, Config* config) {
    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "i:o:dph", long_options, &option_index)) != -1) {
        switch (c) {
            case 'i':
                config->input_file = strdup(optarg);
                break;
            case 'o':
                config->output_file = strdup(optarg);
                break;
            case 'd':
                config->debug_mode = true;
                break;
            case 'p':
                config->profile_mode = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return false;
            case '?':
                return false;
            default:
                fprintf(stderr, "Unknown option: %c\n", c);
                return false;
        }
    }

    if (!config->input_file) {
        fprintf(stderr, "Input file is required\n");
        print_usage(argv[0]);
        return false;
    }

    return true;
}

static bool load_binary(const char* filename, Memory* memory, uint64_t* entry_point) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* buffer = malloc(size);
    if (!buffer) {
        fclose(file);
        return false;
    }

    if (fread(buffer, 1, size, file) != size) {
        free(buffer);
        fclose(file);
        return false;
    }

    if (!memory_map(memory, 0x400000, size, PERM_READ | PERM_EXEC)) {
        free(buffer);
        fclose(file);
        return false;
    }

    if (!memory_copy_to(memory, 0x400000, buffer, size)) {
        free(buffer);
        fclose(file);
        return false;
    }

    *entry_point = 0x400000;

    free(buffer);
    fclose(file);
    return true;
}

static void cleanup(JITContext* jit, Memory* memory, RegisterFile* registers, ProfilingContext* profiling) {
    if (jit) jit_destroy(jit);
    if (memory) memory_destroy(memory);
    if (registers) registers_destroy(registers);
    if (profiling) profiling_destroy(profiling);
}