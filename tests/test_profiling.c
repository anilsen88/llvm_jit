#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "../include/profiling.h"

static void do_work(int iterations) {
    volatile int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;
    }
}

static void test_profiler_creation() {
    Profiler* profiler = profiler_create();
    assert(profiler != NULL);
    assert(profiler->event_count == 0);
    assert(profiler->events != NULL);
    assert(profiler->current_event == NULL);
    profiler_destroy(profiler);
}

static void test_event_tracking() {
    Profiler* profiler = profiler_create();
    
    profiler_start_event(profiler, "test_event");
    do_work(1000);
    profiler_end_event(profiler);
    
    assert(profiler->event_count == 1);
    assert(strcmp(profiler->events[0].name, "test_event") == 0);
    assert(profiler->events[0].duration_ns > 0);
    
    profiler_destroy(profiler);
}

static void test_nested_events() {
    Profiler* profiler = profiler_create();
    
    profiler_start_event(profiler, "outer");
    do_work(1000);
    
    profiler_start_event(profiler, "inner1");
    do_work(500);
    profiler_end_event(profiler);
    
    profiler_start_event(profiler, "inner2");
    do_work(500);
    profiler_end_event(profiler);
    
    profiler_end_event(profiler);
    
    assert(profiler->event_count == 3);
    assert(strcmp(profiler->events[0].name, "outer") == 0);
    assert(strcmp(profiler->events[1].name, "inner1") == 0);
    assert(strcmp(profiler->events[2].name, "inner2") == 0);
    
    assert(profiler->events[1].parent_index == 0);
    assert(profiler->events[2].parent_index == 0);
    assert(profiler->events[0].parent_index == -1);
    
    profiler_destroy(profiler);
}

static void test_event_statistics() {
    Profiler* profiler = profiler_create();
    const int iterations = 5;
    
    for (int i = 0; i < iterations; i++) {
        profiler_start_event(profiler, "repeated_event");
        do_work(1000);
        profiler_end_event(profiler);
    }
    
    ProfileEventStats stats;
    profiler_get_event_stats(profiler, "repeated_event", &stats);
    
    assert(stats.call_count == iterations);
    assert(stats.total_duration_ns > 0);
    assert(stats.min_duration_ns > 0);
    assert(stats.max_duration_ns > 0);
    assert(stats.avg_duration_ns > 0);
    assert(stats.max_duration_ns >= stats.min_duration_ns);
    assert(stats.avg_duration_ns >= stats.min_duration_ns);
    assert(stats.avg_duration_ns <= stats.max_duration_ns);
    
    profiler_destroy(profiler);
}

static void test_event_filtering() {
    Profiler* profiler = profiler_create();
    
    profiler_start_event(profiler, "event1");
    do_work(100);
    profiler_end_event(profiler);
    
    profiler_start_event(profiler, "event2");
    do_work(200);
    profiler_end_event(profiler);
    
    profiler_start_event(profiler, "event1");
    do_work(300);
    profiler_end_event(profiler);
    
    // Test filtering events by name
    ProfileEvent* filtered;
    size_t count = profiler_filter_events(profiler, "event1", &filtered);
    assert(count == 2);
    assert(strcmp(filtered[0].name, "event1") == 0);
    assert(strcmp(filtered[1].name, "event1") == 0);
    free(filtered);
    
    profiler_destroy(profiler);
}

static void test_performance_markers() {
    Profiler* profiler = profiler_create();
    
    profiler_mark_start(profiler, "performance_test");
    do_work(10000);
    profiler_mark_end(profiler, "performance_test");
    
    uint64_t duration = profiler_get_marker_duration(profiler, "performance_test");
    assert(duration > 0);
    
    profiler_mark_start(profiler, "test1");
    do_work(1000);
    profiler_mark_start(profiler, "test2");
    do_work(2000);
    profiler_mark_end(profiler, "test2");
    profiler_mark_end(profiler, "test1");
    
    uint64_t duration1 = profiler_get_marker_duration(profiler, "test1");
    uint64_t duration2 = profiler_get_marker_duration(profiler, "test2");
    assert(duration1 > duration2);
    
    profiler_destroy(profiler);
}

static void test_memory_tracking() {
    Profiler* profiler = profiler_create();
    
    profiler_track_allocation(profiler, "heap1", 1024);
    profiler_track_allocation(profiler, "heap2", 2048);
    profiler_track_deallocation(profiler, "heap1", 1024);
    
    MemoryStats mem_stats;
    profiler_get_memory_stats(profiler, &mem_stats);
    
    assert(mem_stats.current_bytes == 2048);
    assert(mem_stats.peak_bytes == 3072);
    assert(mem_stats.total_allocations == 2);
    assert(mem_stats.total_deallocations == 1);
    
    profiler_destroy(profiler);
}

static void test_thread_safety() {
    Profiler* profiler = profiler_create();

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            profiler_start_event(profiler, "thread1");
            do_work(1000);
            profiler_end_event(profiler);
        }
        
        #pragma omp section
        {
            profiler_start_event(profiler, "thread2");
            do_work(1000);
            profiler_end_event(profiler);
        }
    }
    
    assert(profiler->event_count == 2);
    assert(profiler->events[0].thread_id != profiler->events[1].thread_id);
    
    profiler_destroy(profiler);
}

static void test_profiler_reporting() {
    Profiler* profiler = profiler_create();
    
    profiler_start_event(profiler, "main");
    
    profiler_start_event(profiler, "initialization");
    do_work(1000);
    profiler_end_event(profiler);
    
    profiler_start_event(profiler, "processing");
    do_work(2000);
    
    profiler_start_event(profiler, "sub_task");
    do_work(500);
    profiler_end_event(profiler);
    
    profiler_end_event(profiler);
    
    profiler_end_event(profiler);
    
    char report[4096];
    
    size_t len = profiler_generate_text_report(profiler, report, sizeof(report));
    assert(len > 0);
    assert(strstr(report, "main") != NULL);
    assert(strstr(report, "initialization") != NULL);
    assert(strstr(report, "processing") != NULL);
    assert(strstr(report, "sub_task") != NULL);
    
    len = profiler_generate_json_report(profiler, report, sizeof(report));
    assert(len > 0);
    assert(strstr(report, "\"name\":\"main\"") != NULL);
    assert(strstr(report, "\"duration\":") != NULL);
    
    profiler_destroy(profiler);
}

int main() {
    printf("Running profiler tests...\n");
    
    test_profiler_creation();
    test_event_tracking();
    test_nested_events();
    test_event_statistics();
    test_event_filtering();
    test_performance_markers();
    test_memory_tracking();
    test_thread_safety();
    test_profiler_reporting();
    
    printf("All profiler tests passed!\n");
    return 0;
} 