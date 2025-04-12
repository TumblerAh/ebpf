#ifndef __SWITCH_MONITOR_H
#define __SWITCH_MONITOR_H


#define TASK_COMM_LEN 16
#define MAX_CPUS 256


enum sched_event_type{
    SCHED_EVENT_SWITCH_OUT,
    SCHED_EVENT_SWITCH_IN,
    SCHED_EVENT_MIGRATE //判断有没有发生事件迁移
};

// 调度事件数据结构
struct sched_event {
    __u64 timestamp;        // 事件时间戳
    __u32 pid;              
    __u32 tid;              
    __u32 prev_cpu;         // 之前运行的CPU
    __u32 new_cpu;          // 新分配的CPU
    __u64 runtime;          // 本次运行时间
    __u64 vruntime;         // 虚拟运行时间
    __u32 event_type;       // 事件类型
    __s32 prio;             // 优先级
    char comm[TASK_COMM_LEN]; // 任务名称
};

// CPU负载数据结构
struct cpu_load{
    __u64 timestamp;
    __u32 cpu_id;
    __u32 load;
};


#endif