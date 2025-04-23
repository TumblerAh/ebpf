# eBPF入门

[bpf-developer-tutorial/src/1-helloworld/README.zh.md at main · eunomia-bpf/bpf-developer-tutorial · GitHub](https://github.com/eunomia-bpf/bpf-developer-tutorial/blob/main/src/1-helloworld/README.zh.md)

https://nakryiko.com/posts/bcc-to-libbpf-howto-guide/#bpf-skeleton-and-bpf-app-lifecycle

## **ebpf数据流如下图所示**：

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-03-31-19-54-57-image.png)

1. 用C语言编写 eBPF 的代码；

2. 用clang编译器把 C语言代码 编译成 eBPF 字节码；
   
   ```c
   # 生成的文件为ELF文件类型
   clang - target bpf -c minimal.bpf.c -o minimal.bpf.o
   
   # 可以查看elf中的section headers：
   llvm-readelf -S minimal.bpf.o
   
   # 查看clang编译得到的ebpf字节码
   llvm-objdump -d minimal.bpf.o
   
   # 运行
   sudo ./minimal
   ```

3. 通过 bpf 的 syscall 系统调用，把eBPF字节码加载到内核；

4. 内核对eBPF字节码进行安全校验；是否有内存越界风险，复杂度是否过高，是否会进入死循环等；

5. 内核对eBPF字节码即时编译成本地机器码；
   
   ```c
   # 以下命令可以查看加载到内核中的eBPF字节码
   # 打印内核运行中的eBPF id
   sudo bpftool prog show
   
   # 打印 id = 53 的eBPF程序的汇编指令
   sudo bpftool prog dump xlated id 53
   sudo bpftool prog dump xlated id 53 opcodes # 增加 opcode(字节码) 的打印
   
   # JIT 编译成本地机器码，暂时还没办法看到
   ```

6. 把eBPF程序attach到挂接点，比如: system calls, function entry/exit, kernel tracepoints, network events, 等；

7. 挂接点上的eBPF程序被触发运行； eBPF 程序都是被动触发运行的；

```c
# 观察下 eBPF 程序的输出结果
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

maps的作用：

负责内核层和应用层之间的数据交互，应用层通过map获取内核层ebpf程序收集到的数据，应用层把参数通过map传递到内核层的ebpf程序

eBPF map的类型介绍，使用场景，程序示例，请参考：

http://arthurchiao.art/blog/bpf-advanced-notes-1-zh/

## libbpf-bootstrap

源码下载

```
git clone --recurse-submodules https://github.com/libbpf/libbpf-bootstrap
```

**libbpf**

是对bpf syscall(系统调用) 的基础封装，提供了 open, load, attach, maps操作, CO-RE, 等功能：

- open：从elf文件中提取 eBPF的字节码程序，maps等；
  
  ```c
  LIBBPF_API struct bpf_object *bpf_object__open(const char *path);
  ```

- load：把 eBPF字节码程序，maps等加载到内核

```c
LIBBPF_API int bpf_object__load(struct bpf_object *obj);
```

- attach：把eBPF程序attch到挂接点

```c
LIBBPF_API struct bpf_link *bpf_program__attach(......);
LIBBPF_API struct bpf_link *bpf_program__attach_perf_event(......);
LIBBPF_API struct bpf_link *bpf_program__attach_kprobe(......);
LIBBPF_API struct bpf_link *bpf_program__attach_uprobe(......);
LIBBPF_API struct bpf_link *bpf_program__attach_ksyscall(......);
LIBBPF_API struct bpf_link *bpf_program__attach_usdt(......);
LIBBPF_API struct bpf_link *bpf_program__attach_tracepoint(......);
......
```

- maps的操作

```c
LIBBPF_API int bpf_map__lookup_elem(......);
LIBBPF_API int bpf_map__update_elem(......);
LIBBPF_API int bpf_map__delete_elem(......);
......
```

- CO-RE(Compile Once – Run Everywhere)

​ CO-RE可以实现eBPF程序一次编译，在不同版本的内核中正常运行；下面的章节会详细展开讲；

```c
bpf_core_read(dst, sz, src)
bpf_core_read_user(dst, sz, src)
BPF_CORE_READ(src, a, ...)
BPF_CORE_READ_USER(src, a, ...)
```

**libbpf-bootstrap：**

基于 libbpf 开发出来的eBPF内核层代码，通过bpftool工具直接生成用户层代码操作接口，极大减少开发人员的工作量；

eBPF一般都是分2部分：内核层代码 + 用户层代码

内核层代码：跑在内核层，负责实现真正的eBPF功能

用户层代码：跑在用户层，负责 open, load, attach eBPF内核层代码到内核，并负责用户层和内核层的数据交互；

所以在libbpf-bootstrap框架中开发一个eBPF功能，一般都需要两个基础代码文件，比如需要开发minimal的eBPF程序，需要minimal.bpf.c 和 minimal.c 两个文件，如果这两个文件还需要公共的头文件，可以定义头文件：minimal.h

minimal.bpf.c 是内核层代码，被 clang 编译器编译成 minimal.tmp.bpf.o

bpftool 工具通过 minimal.tmp.bpf.o 自动生成 minimal.skel.h 头文件：

```shell
clang -g -O2 -target bpf -c minimal.bpf.c -o minimal.tmp.bpf.o
bpftool gen object minimal.bpf.o minimal.tmp.bpf.o
bpftool gen skeleton minimal.bpf.o > minimal.skel.h
```

minimal.skel.h头文件中包含了minimal.bpf.c对应的elf文件数据，以及用户层需要的open,load,attach等接口；

**eBPF程序的生命周期**

4个阶段：open,load,attach,destory

- open阶段：从clang编译器编译得到的eBPF程序elf文件中抽取maps，eBPF程序，全局变量等，但是还没有在内核中创建，所以还可以对maps，全局变量进行必要的修改。

- load阶段：maps，全局变量在内核中被创建，eBPF字节码程序加载到内核中，并进行校验，但在这个阶段，eBPF程序虽然存在内核中，但是还不会被运行，还可以对内核中的maps进行初始状态的赋值

- attach阶段：eBPF程序被attach到挂接点，eBPF相关功能开始运行，比如：eBPF程序被触发运行，更新maps，全局变量等 

- destory阶段：eBPF程序被detached，eBPF用到的资源将会被释放
  
  在libbpf-bootstrap中，4个阶段对应的用户层接口：
  
  ```
  // open 阶段，xxx：根据eBPF程序文件名而定
  xxx_bpf__open(...);
  
  // load 阶段，xxx：根据eBPF程序文件名而定
  xxx_bpf__load(...);
  
  // attach 阶段，xxx：根据eBPF程序文件名而定
  xxx_bpf__attach(...);
  
  // destroy 阶段，xxx：根据eBPF程序文件名而定
  xxx_bpf__destroy(...);
  
  //以上接口都是libbpf-bootstrap根据开发人员的eBPF文件自动生成，
  ```

### kprobe

可以在几乎所有的函数中动态插入探测点，利用注册的回调函数，知道内核函数是否被调用，被调用上下文，入参以及返回值；

原理步骤如下：

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-04-01-15-30-55-image.png)

- 如果用户没有注册kprobe探测点，指令流：`指令1(instr1)` 顺序执行到 `指令4(instr4)`
- 如果用户注册一个kprobe探测点到`指令2(instr2)`，`指令2`被备份，并把`指令2`的入口点替换为断点指令，断点指令是CPU架构相关，如x86-64是int3，arm是设置一个未定义指令；
- 当CPU执行到断点指令时，触发一个 `trap`，在`trap`流程中，
  - 首先，执行用户注册的 `pre_handler` 回调函数；
  - 然后，单步执行前面备份的`指令2(instr2)`；
  - 单步执行完成后，执行用户注册的 `post_handler` 回调函数；
  - 最后，执行流程回到被探测指令之后的正常流程继续执行；

kretprobe是在kprobe基础上实现的

**通过示例代码来了解kprobe的使用方法**：

```
libbpf-bootstrap/examples/c/ 目录下的：
(功能：操作系统在删除文件时，会打印删除文件的文件名以及返回值)
kprobe.bpf.c
kprobe.c
```

首先是头文件`#include "vmlinux.h"` 

```c
//在内核层的eBPF程序中，包含 vmlinux.h 头文件就说明需要使用 CO-RE 功能, 否则就是不使用
#include "vmlinux.h"


//使用CO-RE需要内核打开 CONFIG_DEBUG_INFO_BTF 配置选项，如果内核版本过低，不支持这个配置选项，
//就不要使用 CO-RE，即不要包含 vmlinux.h 头文件
```

```hash
# 可以使用下面的命令在当前目录下生成vmlinux.h文件
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```

编译前工作

```hash
# 确定系统结构
uname -m 

# 在架构为x86_64下进行编译（-O2：优化代码，减少指令数；-g：保留调试信息；-D__TARGET_ARCH_xxx：指定目标架构;-I:确保编译器可以找到vmlinux.h）
clang -g  -O2 -target bpf -D__TARGET_ARCH_x86  -I. -c kprobe.bpf.c -o kprobe.tmp.bpf.o

# bpftool工具通过kprobe.tmp.bpf.o 自动生成kprobe.skel.h头文件
bpftool gen object kprobe.bpf.o kprobe.tmp.bpf.o
bpftool gen skeleton kprobe.bpf.o > kprobe.skel.h
```

关于编译

```haskell
# 编译用户空间程序
clang -Wall -I. -c kprobe.c -o kprobe.o
# 链接生成可执行文件
clang -Wall kprobe.o -L/usr/lib64 -lbpf -lelf -o kprobe
```

**完整的编译及运行如下：**

```haskell
# 1. 生成vmlinux.h
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

# 2. 编译eBPF程序
clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -I. -c kprobe.bpf.c -o kprobe.tmp.bpf.o
bpftool gen object kprobe.bpf.o kprobe.tmp.bpf.o
bpftool gen skeleton kprobe.bpf.o > kprobe.skel.h

# 3. 编译用户空间程序
clang -Wall -I. -c kprobe.c -o kprobe.o
clang -Wall kprobe.o -L/usr/lib64 -lbpf -lelf -o kprobe

# 4. 运行
sudo ./kprobe
```

在新的终端界面1输入

```haskell
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

在另一个界面输入

```haskell
touch a.txt
rm a.txt
```

可以看到终端界面1打印出

```haskell
 <...>-720097  [011] ...21 412509.113253: bpf_trace_printk: KPROBE ENTRY pid = 720097, filename = a.txt

 <...>-720097  [011] ...21 412509.113466: bpf_trace_printk: KPROBE EXIT: pid = 720097, ret = 0
```

**kprobe示例代码的实现逻辑**

在kprobe.bpf.c 中

```c
SEC("kprobe/do_unlinkat")    //在内核的 do_unlinkat 入口处注册一个 kprobe 探测点
SEC("kretprobe/do_unlinkat") //在内核的 do_unlinkat 返回时注册一个 kretprobe 探测点

// 使用 BPF_KPROBE 和 BPF_KRETPROBE 宏来定义探测点的回调函数
```

kprobe.c 中的 open, load, attach

一些系统调用

下面对libbpf中的所有example进行分析和学习：

### bootstrap

- BPF_MAP_TYPE_RINGBUF
  
  （高频事件通知、系统跟踪和日志记录、遥测数据收集、实时监控系统）
  
  一种BPF映射类型，它提供了内核态与用户态之间高性能的单生产者/单消费者环形缓冲区数据交换机制。
  
  什么是环形缓冲区结构：循环写入的缓冲区，当满时会覆盖旧数据

### fentry

fentry是Linux内核函数入口跟踪机制

### uprobe

是linux内核提供的一种动态追踪技术，允许在用户空间程序的任意位置（函数入口、偏移地址等）插入探针，用于监控和调试应用程序的行为。

工作原理就是通过替换目标指令为断点指令，触发内核回调处理程序。

![](C:\Users\86137\AppData\Roaming\marktext\images\2025-04-20-21-40-36-image.png)

与kprobe的区别就是kprobe作用于内核空间函数/指令，uprobe作用于用户空间函数/指令。

在应用场景上面，kprobe可以分析系统调用、调度器网络栈延迟；uprobe可以追踪malloc、pthread等调用。

bpf_program__attach_uprobe(prog, false, pid, binary_path, offset);

| `prog` | bpf_program 指针（你定义的 `.bpf.c` 中的函数） |
| ------ | ---------------------------------- |

| `false` | `false = uprobe`（函数入口），`true = uretprobe`（函数返回） |
| ------- | ----------------------------------------------- |

| `pid` | 目标进程 PID（-1 表示所有进程） |
| ----- | ------------------- |

| `binary_path` | ELF 路径 |
| ------------- | ------ |

| `offset` | 函数偏移地址（一般是 0，表示根据符号自动解析） |
| -------- | ------------------------ |

BPF_MAP_TYPE_PERCPU_ARRAY是每个cpu独立数组类型的映射，核心设计目标就是高性能统计和避免锁竞争，特别适合于多核环境下收集每CPU的独立数据。使用这个的时候，系统上每个CPU核都会有该数组的一个独立副本，数据在内存中按照CPU编号对齐存储，然后在eBPF程序运行时，会自动访问当前运行CPU对应的数组副本，不需要手动知道CPU编号，然后因为每个CPU操作自己的副本，因此不需要加锁，适合高频更新的计数器场景。

BPF_MAP_TASK_STORAGE每个任务独立存储

明天需要解决的是用户程序代码怎么和内核调度程序结合起来。然后我需要检测用户程序代码。还是内核调度程序
