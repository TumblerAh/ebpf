#include <stdio.h>
#include <omp.h>
#include <unistd.h>
#include <sys/syscall.h>

struct worker_stats {
    int app_id;
    int thread_id;
    unsigned long iterations;
    volatile int active;
}__attribute__((aligned(64))) stats[16];