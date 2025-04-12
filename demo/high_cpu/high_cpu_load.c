// high_cpu_load.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

// CPU密集型工作函数
void *cpu_intensive_task(void *arg) {
    unsigned long iterations = 0;
    double dummy_result = 0.0;
    
    // 获取线程ID
    int thread_id = *(int *)arg;
    printf("thread %d started\n", thread_id);
    
    // 无限循环制造负载
    while(1) {
        // 混合整数和浮点运算
        for(int i=0; i<100000; i++) {
            dummy_result += sqrt(i * thread_id + 1.0);
            dummy_result = (dummy_result > 1000.0) ? 0.0 : dummy_result;
        }
        iterations++;
        
        // 每隔百万次迭代打印一次(可选)
        if(iterations % 100 == 0) {
            printf("thread %d have finished  %lu iterations\n", thread_id, iterations);
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // 默认使用系统核心数
    int num_threads = 12;
    
    // 允许用户指定线程数
    if(argc > 1) {
        num_threads = atoi(argv[1]);
        if(num_threads <= 0) num_threads = 1;
    }
    
    printf("start %d number cpu \n", num_threads);
    
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    int *thread_ids = malloc(num_threads * sizeof(int));
    
    // 创建线程
    for(int i=0; i<num_threads; i++) {
        thread_ids[i] = i+1;
        if(pthread_create(&threads[i], NULL, cpu_intensive_task, &thread_ids[i])) {
            perror("failed");
            return 1;
        }
    }
    
    // 等待线程(实际上不会返回)
    for(int i=0; i<num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 清理(实际上不会执行到这里)
    free(threads);
    free(thread_ids);
    return 0;
}