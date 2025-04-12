# include <omp.h>
# include<stdio.h>

static long sum_steps = 10000;
double step;

#define NUM_THREADS 2

int main(){
 
  #pragma omp parallel 
    # pragma omp single 

    
}