#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1000000   // ѭ������
#define REPEATS 10  // �ظ����Դ���

// ģ���б仯�Ĺ�������
void workload(int i) {
    volatile double x = i * 3.14159;
    for (int j = 0; j < (i % 100); j++) {
        x = x * 1.001;
    }
}

int main() {
    double start, end;
    // ���Բ�ͬ��chunk��С
    int chunk_sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    int num_chunks = sizeof(chunk_sizes)/sizeof(chunk_sizes[0]);
    
    printf("Chunk_size,static,dynamic,guided\n");
    
    for (int c = 0; c < num_chunks; c++) {
        int chunk = chunk_sizes[c];
        double static_time = 0, dynamic_time = 0, guided_time = 0;
        
        // ��ÿ�ֵ������ͽ��в���
        for (int r = 0; r < REPEATS; r++) {
            start = omp_get_wtime();
            #pragma omp parallel for schedule(static, chunk)
            for (int i = 0; i < N; i++) {
                workload(i);
            }
            static_time += omp_get_wtime() - start;
            
            start = omp_get_wtime();
            #pragma omp parallel for schedule(dynamic, chunk)
            for (int i = 0; i < N; i++) {
                workload(i);
            }
            dynamic_time += omp_get_wtime() - start;
            
            start = omp_get_wtime();
            #pragma omp parallel for schedule(guided, chunk)
            for (int i = 0; i < N; i++) {
                workload(i);
            }
            guided_time += omp_get_wtime() - start;
        }
        
        // ���ƽ����ʱ
        printf("%d,%.6f,%.6f,%.6f\n", 
               chunk, 
               static_time/REPEATS, 
               dynamic_time/REPEATS, 
               guided_time/REPEATS);
    }
    
    return 0;
}