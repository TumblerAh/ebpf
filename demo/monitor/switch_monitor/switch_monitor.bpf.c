#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "switch_monitor.h"

#define OMP_WORKER_PREFIX "app_worker"
#define PREFIX_LEN sizeof(OMP_WORKER_PREFIX)-1


char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
    __uint(type,BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries,1<<24);
}events SEC(".maps");


//记录线程开始时间
struct {
    __uint(type,BPF_MAP_TYPE_HASH);
    __uint(max_entries,10240);
    __type(key,u32); //tid
    __type(value,u64); //timestamp when scheduled in
}thread_start SEC(".maps"); 


struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);       // tid
    __type(value, u64);     // vruntime at last schedule
} thread_vruntime SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct cpu_load);
} cpu_loads SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);      // tid
    __type(value, u32);    // last_cpu
} task_last_cpu SEC(".maps"); //用来跟踪最后一次运行CPU

SEC("tp_btf/sched_switch")
int BPF_PROG(sched_switch,bool preempt,struct task_struct * prev,struct task_struct *next)
{

    char prev_comm[TASK_COMM_LEN];
    char next_comm[TASK_COMM_LEN];
    
    // 获取任务名称
    BPF_CORE_READ_STR_INTO(&prev_comm, prev, comm);
    BPF_CORE_READ_STR_INTO(&next_comm, next, comm);
    
    // 检查是否是目标线程
    bool monitor_prev = (bpf_strncmp(prev_comm, PREFIX_LEN, OMP_WORKER_PREFIX) == 0);
    bool monitor_next = (bpf_strncmp(next_comm, PREFIX_LEN, OMP_WORKER_PREFIX) == 0);
    
    if (!monitor_prev || !monitor_next) {
        return 0;  // 跳过非目标线程
    }

    //按照换出和换入任务进行记录
    u32 prev_tid = BPF_CORE_READ(prev, pid);
    u32 next_tid = BPF_CORE_READ(next, pid);
    u64 ts = bpf_ktime_get_ns();

    u64 prev_vruntime = BPF_CORE_READ(prev, se.vruntime);
    u64 next_vruntime = BPF_CORE_READ(next, se.vruntime);
    u32 prev_prio = BPF_CORE_READ(prev, prio);
    u32 next_prio = BPF_CORE_READ(next, prio);

    //监控被换出任务
    struct sched_event event_out = {
        .timestamp = ts,
        .pid = BPF_CORE_READ(prev,tgid),
        .tid = prev_tid,
        .prev_cpu = bpf_get_smp_processor_id(),
        .new_cpu = (u32) - 1,
        .vruntime = prev_vruntime,
        .event_type = SCHED_EVENT_SWITCH_OUT,
        .prio = prev_prio
    };

    bpf_probe_read_kernel_str(&event_out.comm,sizeof(event_out.comm),prev->comm);
    u64 *start_time = bpf_map_lookup_elem(&thread_start,&prev_tid);
    if(start_time){
        event_out.runtime = ts - *start_time;
        bpf_map_delete_elem(&thread_start,&prev_tid);
    }

    u32 last_cpu = bpf_get_smp_processor_id();
    bpf_map_update_elem(&task_last_cpu,&prev_tid,&last_cpu,BPF_ANY);
    
    //send event
    bpf_ringbuf_output(&events,&event_out,sizeof(event_out),0);

    //监控被换入的任务
    struct sched_event event_in = {
        .timestamp = ts,
        .pid = BPF_CORE_READ(next,tgid),
        .tid = next_tid,
        .prev_cpu = (u32) - 1,
        .new_cpu = bpf_get_smp_processor_id(),
        .vruntime = next_vruntime,
        .event_type = SCHED_EVENT_SWITCH_IN,
        .prio = next_prio
    };

    u32 *prev_cpu_ptr = bpf_map_lookup_elem(&task_last_cpu, &next_tid);
    if (prev_cpu_ptr) {
        event_in.prev_cpu = *prev_cpu_ptr;  // 设置 prev_cpu
    }
    bpf_probe_read_kernel_str(&event_in.comm,sizeof(event_in.comm),next -> comm);

    //记录线程开始时间
    u64 start_ts = ts;
    bpf_map_update_elem(&thread_start,&next_tid,&start_ts,BPF_ANY);

    //检查是否为迁移事件
    u64 *last_vruntime = bpf_map_lookup_elem(&thread_vruntime, &next_tid);
    if (last_vruntime && *last_vruntime != next->se.vruntime) {
        struct sched_event migrate_event = event_in;
        migrate_event.event_type = SCHED_EVENT_MIGRATE;
        bpf_ringbuf_output(&events, &migrate_event, sizeof(migrate_event), 0);
    }

     // 更新vruntime记录
     bpf_map_update_elem(&thread_vruntime, &next_tid, &next_vruntime, BPF_ANY);
    
     // 发送换入事件
     bpf_ringbuf_output(&events, &event_in, sizeof(event_in), 0);
     
     return 0;
}

SEC("tp_btf/sched_stat_runtime")
int BPF_PROG(sched_stat_runtime, struct task_struct *task, u64 runtime, u64 vruntime)
{
    u32 cpu = bpf_get_smp_processor_id();
    u64 ts = bpf_ktime_get_ns();

    // 计算 CPU 负载（假设采样间隔 1ms）
    static __u64 last_update[MAX_CPUS] = {0};
    static __u64 cpu_time[MAX_CPUS] = {0};

    if (last_update[cpu] == 0) {
        last_update[cpu] = ts;
        return 0;
    }

    cpu_time[cpu] += runtime;

    // 每 1ms 计算一次 CPU 负载
    if (ts - last_update[cpu] >= 1000000) {
        __u64 load = (cpu_time[cpu] * 100) / (ts - last_update[cpu]);
        
        struct cpu_load cl = {
            .timestamp = ts,
            .cpu_id = cpu,
            .load = load,
        };
    
        bpf_ringbuf_output(&events, &cl, sizeof(cl), 0);
        
        last_update[cpu] = ts;
        cpu_time[cpu] = 0;
    }

    return 0;
}