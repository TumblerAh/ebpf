# Perf

## 基础介绍

*linux内核追踪框架：ftrace，**perf**，eBPF，System Tap ，sysdig*

 perf provide a framework  for all things performance analysis.It covers hardware level (CPU/PMU,Performance Monitoring Unit) features and software features(software counter.tracepoints) as well.

**perf事件：**

perf事件类型：可以分为硬件和软件（预定义事件、自定义事件、eBPF）

perf事件内容：寄存器（IP），堆栈（用户、内核、堆栈内存dump）、Last Branch Record、时间，线程ID，进程ID

perf事件收集：以周期进行数据收集，RingBuffer大小、覆盖方式，PMU时分复用（group）

perf事件处理：record（记录到文件perf.data)；report（读取perf.data并以CUI方式展示）；stat（统计事件个数）；script（脚本自定义处理）；trace（live输出事件——strace）；probe（自定义软件事件）；top（类似top命令）；list（列出事件）等..

可选参数：

1. **-e** ：选择性能事件。可以是 CPU 周期、缓存未命中等。使用 `perf list` 命令来查看所有可用的事件。
2. **-a**：记录整个系统的性能数据，而不仅仅是一个进程。这对于分析系统级的性能问题非常有用。
3. **-p** ：记录指定进程的性能数据。只关注某一个特定进程。
4. **-t** ：记录指定线程的性能数据。用于分析多线程程序中某个特定线程的性能。
5. **-o** ：指定输出文件名。默认情况下，perf 会将数据保存在当前目录下的 `perf.data` 文件中。
6. **-g**：记录函数调用关系（调用栈）。这对于分析程序中函数调用和热点非常有帮助。
7. **-c** ：设置事件的采样周期。例如，每发生1000次事件采样一次。
8. **-F** ：设置事件的采样频率。例如，每秒采样1000次。

----

**perf使用场景**：

- 寻找热点函数，定位性能瓶颈
  
  这里解释一下，perf是通过使用采样来定位热点函数的。通过基于事件的周期性采样来统计程序中哪些函数或指令消耗了最多的CPU时间，从而定位性能瓶颈。一般是先设置采样事件（比如说cycles，instructions）和频率（-F），之后CPU在运行程序时，PMU在指定事件达到阈值时触发中断，中断发生时，记录当前的IP和调用栈，最后汇总采样点，生成统计报告

- perf可以用来分析CPU cache、CPU迁移、分支预测、指令周期等各种硬件事件

- perf也可以对感兴趣的事件进行动态追踪

## 基础用法

我们通过对下面的代码进行性能分析来介绍用法：

```c
# include<stdio.h>
# include<sys/types.h>
# include<unistd.h>
# include<math.h>

void for_loop(){
    for (int i=0;i<1000;i++){
        for(int j = 0;j<10000;j++){
            int x = sin(i) + cos(j);
        }
    }
}

void loop_small(){
    for(int i=0;i<10;i++){
        for_loop();
    }
}
void loop_big(){
    for(int i=0;i<100;i++){
        for_loop();
    }
}
int main(){
    printf("pid = %d\n",getpid());
    loop_big();
    loop_small();
}
```

对上述代码进行编译 `g++ -o perftest perftest.c`，运行可以得到对应的`pid`

**`perf record`** 采样程序运行时的调用栈，生成火焰图或热点函数列表。

```
perf record -p [pid] -a -g -F 999 -- sleep 10
# -- sleep 10 表示采样时间执行10
```

**`perf report`** 数据会自动记录在perf.data中

```haskell
Samples: 991  of event 'cycles:P', Event count (approx.): 24111747121
  Children      Self  Command   Shared Object      Symbol
+  100.00%     0.00%  perftest  perftest           [.] _start                                                                                            ◆
+  100.00%     0.00%  perftest  libc.so.6          [.] __libc_start_main                                                                                 ▒
+  100.00%     0.00%  perftest  libc.so.6          [.] 0x00007b893dd3d488                                                                                ▒
+  100.00%     0.00%  perftest  perftest           [.] main                                                                                              ▒
+  100.00%     0.00%  perftest  perftest           [.] loop_big()                                                                                        ▒
+   95.59%     6.91%  perftest  perftest           [.] for_loop()                                                                                        ▒
+   44.70%     7.31%  perftest  perftest           [.] __gnu_cxx::__enable_if<std::__is_integer<int>::__value, double>::__type std::sin<int>(int)        ▒
+   44.68%     5.91%  perftest  perftest           [.] __gnu_cxx::__enable_if<std::__is_integer<int>::__value, double>::__type std::cos<int>(int)        ▒
+    4.11%     2.21%  perftest  perftest           [.] sin@plt                                                                                           ▒
+    3.71%     3.71%  perftest  libm.so.6          [.] 0x00000000000808fe                                                                                ▒
+    3.71%     0.00%  perftest  libm.so.6          [.] 0x00007b893df88903                                                                                ▒
+    3.41%     3.41%  perftest  libm.so.6          [.] 0x000000000008114e                                                                                ▒
+    3.41%     0.00%  perftest  libm.so.6          [.] 0x00007b893df89153                                                                                ▒
+    3.21%     2.00%  perftest  perftest           [.] cos@plt                                                                                           ▒
+    2.70%     2.70%  perftest  libm.so.6          [.] 0x0000000000080abf                                                                                ▒
+    2.70%     0.00%  perftest  libm.so.6          [.] 0x00007b893df88ac4                         
```

- samples=991表示采样数为991（因为频率设置为99次/s，执行10s），这里对CPU周期事件进行了991次采样，总事件数大约为24111747121

- Children：子函数的累计开销百分比（包含调用链中所有下层函数的耗时），比如说函数__start的Children是100%，表示整个程序的执行时间都归因于它，因为他是程序的入口

- Self：当前函数自身的开销百分比（不包含子函数），就拿我们这个代码来说，self指的是当前函数内部直接执行的代码所占的百分比，不包括调用的sin和cos，统计时间包括：循环控制，变量赋值，以及调用sin/cos的跳转指令，但是不包括sin/cos内部计算时间，真正的计算发生在libm.so.6中，self仅统计跳转到sin/cos之前的少量指令时间
  
  这里需要区分一下Children和Self的区别，可以通过`man perf report` 查找官方文档（内含有一些例子进行说明）：

```
The overhead can be shown in two columns as Children and Self when perf collects callchains. The self overhead is simply calculated by adding all
       period values of the entry - usually a function (symbol). This is the value that perf shows traditionally and sum of all the self overhead values
       should be 100%.

       The children overhead is calculated by adding all period values of the child functions so that it can show the total overhead of the higher level
       functions even if they don’t directly execute much. Children here means functions that are called from another (parent) function.

       It might be confusing that the sum of all the children overhead values exceeds 100% since each of them is already an accumulation of self overhead
       of its child functions. But with this enabled, users can find which function has the most overhead even if samples are spread over the children.
```

- Command：进程名称

- Shared Object：函数所在的共享库或可执行文件。

- Symbol：函数名或符号地址

`perf report`记录的数据也可以通过**火焰图**来进行分析。

因为要导入仓库，这里就没有进行实操，内容来自网络理论知识：

火焰图解读：x轴表示时间占比，越宽的函数消耗越多CPU；y轴表示调用栈深度（上层调用下层），大概可以理解为x轴也就是self，y轴也就是Children。

**`perf script`** 可以将`perf record` 采集的性能数据转换为可读的文本格式或其他格式输出。

```c
          perftest 3459949 1623812.780231:          1 cycles:P: 

         ffffffffad202c58 __rcu_read_unlock+0x18 ([kernel.kallsyms>
        ffffffffad3980f0 __perf_event_task_sched_in+0x90 ([kernel>
        ffffffffad183da3 finish_task_switch.isra.0+0x1e3 ([kernel>
        ffffffffadef7b9d __schedule+0x42d ([kernel.kallsyms])
        ffffffffadef8a57 schedule+0x27 ([kernel.kallsyms])
        ffffffffadef1f54 irqentry_exit_to_user_mode+0x144 ([kerne>
        ffffffffae00160a asm_sysvec_apic_timer_interrupt+0x1a ([k>
            7b893df88e37 [unknown] (/usr/lib/libm.so.6)
            5a8c20c53261 __gnu_cxx::__enable_if<std::__is_integer>
            5a8c20c5318d for_loop()+0x24 (/home/lhy/demo/perf_dem>
            5a8c20c53200 loop_big()+0x16 (/home/lhy/demo/perf_dem>
            5a8c20c53232 main+0x24 (/home/lhy/demo/perf_demo/perf>
            7b893dd3d488 [unknown] (/usr/lib/libc.so.6)
            7b893dd3d54c __libc_start_main+0x8c (/usr/lib/libc.so>
            5a8c20c53095 _start+0x25 (/home/lhy/demo/perf_demo/pe>
```

会先输出进程名、PID、时间戳、事件

之后会显示出调用栈的情况，从用户态到内核态的完整调用链（最后一行是当前执行的函数，向上是调用者）

比如上述展示，用户态调用链中程序从__start开始，然后调用main，进入到loop_big和for_loop函数中，[unknown]表示未能解析的符号；内核态调用链中，先是触发中断`sam_syssvec_apic_timer_interrupt`，然后经过调度器，上下文切换等。。

**上述是一些简单的实例，总体而言，可以通过输入`man perf` 后面跟上你想要查询的一些功能，之后会提供官方文档，可以根据提供的定义和实例继续学习。下面是一些可以尝试探索学习的一些比较关键的种子的列举。**

### **1. 统计与计数类**

#### **`perf stat`**

- **功能**：实时统计程序的硬件性能计数器（如 CPU 周期、缓存命中率、分支预测错误等）。

- **用途**：快速定位性能瓶颈（CPU/内存/指令级别）。

- **示例**：
  
  perf stat -e cycles,instructions,cache-misses ./program

- **关键选项**：
  
  - `-e`：指定事件（如 `branch-misses`）。
  
  - `-r N`：重复运行并取平均值。

#### **`perf bench`**

- **功能**：内置的基准测试套件，测试系统基础组件（调度器、内存分配等）。

- **子测试**：
  
  - `sched`：调度器性能（如 `perf bench sched pipe`）。
  
  - `mem`：内存操作（如 `perf bench mem memcpy`）。
  
  - `futex`：锁竞争测试。

- **用途**：评估内核或硬件的基础性能。

---

### **2. 采样与剖析类**

#### **`perf record` / `perf report`**

- **功能**：采样程序运行时的调用栈，生成火焰图或热点函数列表。

- **用途**：定位代码级热点（如哪个函数消耗最多 CPU）。

- **示例**：
  
  perf record -F 99 -g ./program  # 采样
  perf report --stdio             # 文本分析
  perf script | flamegraph.pl > out.svg  # 生成火焰图

- **关键选项**：
  
  - `-F`：采样频率（Hz）。
  
  - `-g`：记录调用栈。

#### **`perf top`**

- **功能**：实时显示系统中消耗 CPU 最多的函数/符号（类似 `top` 命令）。

- **用途**：快速识别系统级性能热点。

- **示例**：
  
  perf top -e cycles:k  # 仅监控内核空间

---

### **3. 追踪与探测类**

#### **`perf probe`**

- **功能**：动态在内核或用户态程序中添加探针（动态追踪点）。

- **用途**：无需修改代码，追踪特定函数/变量的行为。

- **示例**：
  
  perf probe --add 'schedule_timeout time'  # 添加内核探针
  perf trace -e probe:schedule_timeout      # 追踪事件

#### **`perf trace`**

- **功能**：类似 `strace`，但性能开销更低，追踪系统调用和信号。

- **用途**：分析程序与内核的交互。

- **示例**：
  
  perf trace -e 'syscalls:sys_enter_*' ./program

---

### **4. 脚本与自动化类**

#### **`perf script`**

- **功能**：将 `perf record` 的数据转换为可读格式或脚本语言（Python）。

- **用途**：自定义分析采样数据。

- **示例**：
  
  perf script -F time,event,trace > output.txt

#### **`perf annotate`**

- **功能**：将采样结果映射到源代码或汇编指令。

- **用途**：精确分析热点代码的底层实现。

- **示例**：
  
  perf annotate -s function_name

---

### **5. 硬件事件类**

#### **`perf list`**

- **功能**：列出当前系统支持的硬件/软件性能事件。

- **用途**：确定可监控的指标（如缓存事件、CPU 指令）。

- **示例**：
  
  perf list | grep cache  # 查看缓存相关事件

#### **`perf mem`**

- **功能**：分析内存访问模式（如加载/存储延迟）。

- **用途**：优化内存密集型程序。

- **示例**：
  
  perf mem record ./program

---

### **6. 其他高级功能**

#### **`perf kmem`**

- **功能**：分析内核内存分配（slab/page 级别）。

- **用途**：检测内核内存泄漏或碎片化。

#### **`perf lock`**

- **功能**：分析锁竞争（自旋锁、互斥锁等）。

- **用途**：优化多线程程序的同步开销。

#### **`perf c2c`**

- **功能**：检测缓存行竞争（False Sharing 问题）。

- **用途**：优化多核并行程序的缓存效率。

---

### **总结：perf 子命令全景表**

| 子命令           | 核心功能      | 典型应用场景      |
| ------------- | --------- | ----------- |
| `perf stat`   | 硬件计数器统计   | 快速瓶颈定位      |
| `perf record` | 采样生成剖析数据  | 代码级热点分析     |
| `perf probe`  | 动态添加追踪点   | 无侵入式调试内核/应用 |
| `perf trace`  | 低开销系统调用追踪 | 替代 `strace` |
| `perf bench`  | 内置基准测试    | 调度器/内存子系统评测 |
| `perf script` | 转换采样数据为脚本 | 自定义分析逻辑     |

---

### **使用建议**

1. **初级分析**：从 `perf stat` 和 `perf top` 开始，快速定位问题方向。

2. **深度优化**：结合 `perf record` + `perf report` 分析代码热点。

3. **内核调试**：使用 `perf probe` 动态追踪内核函数。

4. **基准测试**：利用 `perf bench` 对比不同内核版本或配置的性能。
