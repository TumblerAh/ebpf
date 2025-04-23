# OpenMP相关知识点

## 并行计算

```cpp
# include <omp.h>
```

需要使用编译器（GCC）来编译代码，同时为了使得编译器可以识别OpenMP指令，需要对编译器使用-fopenmp选项来创建一个多线程程序。//gcc -fopenmp hello.c

此时系统将会提供默认线程数，并且线程是并行执行的。那么我们的目标就是确保所有可能的交错都可以产生正确的结果。

多核系统的内存比SMP系统的内存复杂很多，所有处理器共享内存，内存以单个地址空间的形式向它们公开。

缓存中的内存被映射到DRAM中的地址。

大多数操作系统都支持称为pthreads的IEEE POSIX线程模型，pthreads编程的第一步是将要在线程中执行的代码隔离为一个函数。

## 表演语言

挂钟时间：我们亲身经历的时间。高性能计算（HPC）主要以浮点数运算为中心。这个度量单位是FLOPS（每秒浮点运算次数）。

加速比：在串行程序上运行时间/在p个处理器上运行的并行程序的运行时间。

阿姆达尔定律

## 负载均衡

显示、自动、静态、动态

显示-静态：程序员根据程序中的逻辑定义块。在编译时就已经是固定的了。

显示-动态：程序员将逻辑写入代码中，以确定如何分配工作。不定时暂停并重新访问该逻辑以动态的重新分配负载。

自动-动态：程序创建一系列块并将其放入到队列中。线程抓取一个工作快，然后完成后该工作块会返回到队列中。

自动-静态：静态负载平衡策略本质是在工作开始之前固定工作分配。

## 什么是OpenMP

OpenMP是用于编写并行程序的应用程序编程接口。

OpenMP使用操作系统提供的任何线程模型，在大多数情况下是pthreads。操作系统层之上是OpenMP运行时系统。由支持执行OpenMP程序的低级库和软件组成。

OpenMP程序的结构：

编译器根据指令对与指令关联的代码块执行某些操作。概述代码：编译器在编译期间根据程序中的语句创建函数。程序员永远不会明确看到这个函数，编译器创建该函数并在编译过程中生成的代码中调用该函数。

代码块将从块的顶部进入并从快的末尾退出。换句话说，程序不会跳入快的中间或跳出块的中间。OpenMP将词代码块称为结构化块。

## SPMD设计模式

单程序多数据设计模式

启动两个或多个执行相同代码的线程；

每个线程确定自己的ID和团队中的线程数；

使用团队中的ID和线程数在线程之间划分工作。

我们需要将循环拆分到线程之间，称之为循环迭代的循环分布。目前提供两种思路：

第一种是将循环按照线程数间隔的样子分配给每个线程，比如有四个线程，那么我们将0，4，8 分给第一个线程， 1，5...继续往后分。然后注意判断循环操作中哪些是每个线程自带的数据，哪些是需要公共享有的数据，在进行数据的定义。

第二种是将每一次循环的步骤分配到每个线程中，然后通过id*步骤数与(id+1)*步骤数的形式找到当前线程需要执行的代码。

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-21-16-15-02-image.png)

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-21-16-21-28-QQ_1742545286806.png)

### schedule

如何将循环中的迭代分配给不同的线程

static：迭代被分成大小相等的块（或尽可能相等），适用于迭代执行时间相对均匀的情况

dynamic：迭代被分成大小为chunk_size的块，线程在完成当前块后，会动态的请求下一个块，适用于迭代执行时间不均匀的情况。

guided：类似于dynamic，但是块的大小会逐渐减小，初始块大小比较大，随着迭代的进行，块大小逐渐减少，适用于迭代时间不均匀，但希望减少调度开销的情况

auto：调度策略由编译器或运行时系统自动决定，适用于不确定哪种调度策略最优的情况

runtime：调度策略在运行时通过环境变量OMP_SCHEDULE决定，可以在不修改代码的情况下调整调度策略

  &# pragma omp for schedule(runtime)

export OMP_SCHEDULE = "dynamic,5"

work sharing construct

& #pragma omp for  会将循环的迭代分配给多个线程并行执行，每个线程会执行一部分迭代，从而加速整个循环的执行，通常需要与#pragma omp parallel 结合起来一起适用

reduction 是openmp中的一个重要子句，用于在并行区域中对变量进行规约操作。规约操作指的是将多个线程中的局部变量通过某种运算符合并成为一个全局结果。reduction子句会自动处理多个线程对共享变量的访问，避免数据竞争问题。

& #pragma omp parallel for reduction (operator:variable)

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-22-20-47-50-image.png)

barrier它的作用是确保所有线程在执行到 `barrier` 时都会停下来，直到所有线程都到达这个点，然后才能继续执行后续代码。

同时openmp中的一些指令，比如说#pragma omp for 后面会自动插入一个隐式的barrier

可以食用nowait子句来移除这个隐式barrier

single：用于指定一个代码块，该代码块仅由并行区域中的一个线程执行，其他线程会跳过这个代码块，指导执行该代码块的线程完成。默认情况下，single指令的末尾会有一个隐式的barrier，也就是其他线程会等待执行single代码块的线程完成。

lock 确保同一时间只有一个线程可以访问共享资源，通过锁机制可以防止多个线程同时修改共享资源

openmp提供了两种锁机制，简单锁（omp_lock_t）嵌套锁（omp_next_lock_t）允许同一线程多次加锁

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-22-22-34-07-image.png)

锁机制会引入额外的开销，尤其是在高竞争的情况下，可能会导致线程频繁等待。如果可以，尽量适用更轻量级的同步机制（如原子操作）

omp_weight_policy:在openmp中,omp_weight_policy是一个任务调度与负载均衡相关的概念，用于指定任务的权重分配策略。它通常与任务调度和任务依赖结合使用，以优化并行任务的执行效率。

omp_weight_policy用于指定任务的权重分配策略，帮助openmp运行时系统更好的调度任务，实现负载均衡，通过合理的权重分配策略，可以减少线程的等待时间，提高并行程序的性能。

1. **`omp_weight_default`**：
   
   - 使用默认的权重分配策略。
   
   - 运行时系统会自动决定任务的权重。

2. **`omp_weight_thread`**：
   
   - 根据线程的负载情况分配权重。
   
   - 负载较重的线程会分配较少的任务，负载较轻的线程会分配较多的任务。

3. **`omp_weight_core`**：
   
   - 根据 CPU 核心的负载情况分配权重。
   
   - 负载较重的核心会分配较少的任务，负载较轻的核心会分配较多的任务。

4. **`omp_weight_cpu`**：
   
   - 根据 CPU 的负载情况分配权重。
   
   - 负载较重的 CPU 会分配较少的任务，负载较轻的 CPU 会分配较多的任务。

5. **`omp_weight_numa`**：
   
   - 根据 NUMA 节点的负载情况分配权重。
   
   - 负载较重的 NUMA 节点会分配较少的任务，负载较轻的 NUMA 节点会分配较多的任务。

omp_proc_bind

data environment

openmp is a shared memory programming model , most varivables are sitting on the heap and all the threads can see them.But not everything is shared if it's on the stack it is private to a thread . 

显示控制数据环境

private doesn't initialize the variable it creates the temporary but it dosen't give it a initial value ,

-->firstprivate ,lastprivate

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-24-20-23-45-image.png)

tasks are independent units of work,tasks are composed of 

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-24-21-37-26-image.png)

task是一种实现任务并行的指令，运行程序员将代码块显示定义为并行执行的独立任务。当遇到#pragma omp task 时，openmp会立即生成一个任务（但是不一定立即执行），这个任务会被放入任务队列，由线程池中的线程异步执行。openmp的运行时系统会自动分配任务给空闲线程，如果所有线程都忙，任务会排队等待直到有线程可用。某个线程从队列中取出任务并执行，任务之间默认没有固定顺序。

  在openmp中，task通常与parallel和single指令搭配使用。这种组合是为了高效生成任务并且合理分配线程资源。理由如下：

1.#pragma omp parallel 

parallel会创建一组线程，线程数量由OMP_NUM_THREADS决定。这些线程共享任务队列，可以并行执行task。

而任务需要线程来执行：task本身只是定义任务，但任务必须由线程执行。parallel提供了执行任务的线程池。避免重复创建线程，如果每次生成任务都创建新线程，开销会很大，parallel创建线程池后，任务可以高效调度。

2.#pragma omp single

确保任务只生成一次，single指定只有一个线程执行该代码块（通常是主线程），如果不加single，所有线程都hi生成任务，导致任务重复生成。

```c
#pragma omp parallel  // 1. 创建线程池
{
    #pragma omp single  // 2. 让一个线程生成任务
    {
        for (int i = 0; i < 10; i++) {
            #pragma omp task  // 3. 生成任务
            { 
                printf("Task %d by thread %d\n", i, omp_get_thread_num()); 
            }
        }
    }
    // 隐式同步：所有线程在这里等待任务完成
}
```

1. `parallel` 创建多个线程（如 4 个线程）。

2. `single` 让 **仅一个线程**（如主线程）执行循环，生成 10 个任务。

3. 所有线程（包括生成任务的线程）从任务队列中 **窃取任务并执行**。

4. 并行区域结束时，所有任务已完成（隐式同步）。

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-25-00-05-20-image.png)

工作负载指的是循环体内每次迭代执行的计算任务，chunk指的是在并行循环中分配给每个线程的一次迭代次数

# 
