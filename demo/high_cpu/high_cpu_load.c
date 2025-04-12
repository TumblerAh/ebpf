// high_cpu_load.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

// CPU�ܼ��͹�������
void *cpu_intensive_task(void *arg) {
    unsigned long iterations = 0;
    double dummy_result = 0.0;
    
    // ��ȡ�߳�ID
    int thread_id = *(int *)arg;
    printf("thread %d started\n", thread_id);
    
    // ����ѭ�����츺��
    while(1) {
        // ��������͸�������
        for(int i=0; i<100000; i++) {
            dummy_result += sqrt(i * thread_id + 1.0);
            dummy_result = (dummy_result > 1000.0) ? 0.0 : dummy_result;
        }
        iterations++;
        
        // ÿ������ε�����ӡһ��(��ѡ)
        if(iterations % 100 == 0) {
            printf("thread %d have finished  %lu iterations\n", thread_id, iterations);
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // Ĭ��ʹ��ϵͳ������
    int num_threads = 12;
    
    // �����û�ָ���߳���
    if(argc > 1) {
        num_threads = atoi(argv[1]);
        if(num_threads <= 0) num_threads = 1;
    }
    
    printf("start %d number cpu \n", num_threads);
    
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    int *thread_ids = malloc(num_threads * sizeof(int));
    
    // �����߳�
    for(int i=0; i<num_threads; i++) {
        thread_ids[i] = i+1;
        if(pthread_create(&threads[i], NULL, cpu_intensive_task, &thread_ids[i])) {
            perror("failed");
            return 1;
        }
    }
    
    // �ȴ��߳�(ʵ���ϲ��᷵��)
    for(int i=0; i<num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // ����(ʵ���ϲ���ִ�е�����)
    free(threads);
    free(thread_ids);
    return 0;
}