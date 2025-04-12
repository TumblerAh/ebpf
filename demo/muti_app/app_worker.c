#include <stdio.h>
#include <omp.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "app_worker.h"
#include<sys/types.h>
#include <stdlib.h>  
#include<sys/prctl.h>
#include<time.h>

#define WORKER_NAME "app_worker"

void worker(int app_id){
    int thread_id = omp_get_thread_num();
    pid_t tid = syscall(SYS_gettid);

    stats[thread_id] = (struct worker_stats){
        .app_id = app_id,
        .thread_id = thread_id,
        .iterations = 0,
        .active = 1
    };
   
    char thread_name[16];
    snprintf(thread_name, sizeof(thread_name), "%s-%d-%d", WORKER_NAME, app_id, thread_id);
    prctl(PR_SET_NAME, thread_name); 
    printf("App %d name %s  Thread %d (TID %d) started\n",app_id, thread_name,thread_id,tid);
    // printf("%d exec",app_id);
    
    unsigned long count = 0;
    int x = 10000;
    while(x--){

        for (int i=0;i<100000;i++){
            count += i % 256;
        }
        stats[thread_id].iterations = count; 

        static unsigned long sleep_counter = 0;
        if (++sleep_counter % 100 == 0) {
            usleep(1000);
        }
       
    }

}

int main(int argc , char*argv[]){

    if(argc < 3){
        printf("Usage: %s<app_id><threads_number>\n",argv[0]);
        return 1;
    }

    int app_id = atoi(argv[1]);
    int threads_number = atoi(argv[2]);

    double start_time = omp_get_wtime();

    char process_name[16];
    snprintf(process_name,sizeof(process_name),"%s-%d",WORKER_NAME,app_id);
    prctl(PR_SET_NAME,process_name);

  
    # pragma omp parallel num_threads(threads_number)
    {
        worker(app_id);
    }
    double end_time = omp_get_wtime();
    printf("APP %d Time take:%.6f seconds\n",app_id,end_time- start_time);
    return 0;
    
}