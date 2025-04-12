#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "process_monitor.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

/* 配置参数 */
const volatile char target_prefix[16] = "omp_worker";  // 要监控的进程名前缀
const volatile unsigned int prefix_len = 10;          // 前缀长度(omp_worker=10)
const volatile unsigned long long min_duration_ns = 0; // 最小持续时间(纳秒)

/* eBPF Maps */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 8192);
    __type(key, pid_t);
    __type(value, u64);  // 存储进程开始时间
} exec_start SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024); 
} rb SEC(".maps");

/* 检查进程名是否匹配目标前缀 */
static inline bool is_target_process(const char *comm)
{
    for (int i = 0; i < prefix_len; i++) {
        if (comm[i] != target_prefix[i]) {
            return false;
        }
    }
    return true;
}

SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx)
{
    struct task_struct *task;
    struct event *e;
    pid_t pid, ppid;
    u64 ts;
    char comm[TASK_COMM_LEN];

    /* 获取当前进程信息 */
    pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(&comm, sizeof(comm));
    


    /* 只处理目标前缀的进程 */
    if (!is_target_process(comm))
        return 0;
    bpf_printk("%d has used exec\n",pid);
    /* 记录进程开始时间 */
    ts = bpf_ktime_get_ns();
    bpf_map_update_elem(&exec_start, &pid, &ts, BPF_ANY);

    /* 准备事件数据 */
    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    /* 填充事件数据 */
    task = (struct task_struct *)bpf_get_current_task();
    BPF_CORE_READ_INTO(&ppid, task, real_parent, tgid);

    e->exit_event = false;
    e->pid = pid;
    e->ppid = ppid;
    e->start_ts = ts;
    e->duration_ns = 0;
    e->exit_code = 0;
    __builtin_memcpy(e->comm, comm, TASK_COMM_LEN);

    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(struct trace_event_raw_sched_process_template *ctx)
{
    struct task_struct *task;
    struct event *e;
    pid_t pid, tid, ppid;
    u64 id, ts, *start_ts, duration_ns = 0;
    char comm[TASK_COMM_LEN];


    /* 获取当前进程信息 */
    id = bpf_get_current_pid_tgid();
    pid = id >> 32;
    tid = (u32)id;
    bpf_get_current_comm(&comm, sizeof(comm));

   

    /* 只处理目标前缀的进程 */
    if (!is_target_process(comm))
        return 0;
    bpf_printk("%d has used exit\n",pid);
    /* 忽略线程退出 */
    if (pid != tid)
        return 0;

    /* 计算进程持续时间 */
    start_ts = bpf_map_lookup_elem(&exec_start, &pid);
    if (start_ts) {
        duration_ns = bpf_ktime_get_ns() - *start_ts;
        bpf_map_delete_elem(&exec_start, &pid);
    }

    /* 如果设置了最小持续时间且不满足则跳过 */
    if (min_duration_ns && duration_ns < min_duration_ns)
        return 0;

    /* 准备事件数据 */
    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    /* 填充事件数据 */
    task = (struct task_struct *)bpf_get_current_task();
    BPF_CORE_READ_INTO(&ppid, task, real_parent, tgid);

    e->exit_event = true;
    e->pid = pid;
    e->ppid = ppid;
    e->start_ts = start_ts ? *start_ts : 0;
    e->duration_ns = duration_ns;
    e->exit_code = (BPF_CORE_READ(task, exit_code) >> 8) & 0xff;
    __builtin_memcpy(e->comm, comm, TASK_COMM_LEN);

    bpf_ringbuf_submit(e, 0);
    return 0;
}