#ifndef __PROCESS_MONITOR_H
#define __PROCESS_MONITOR_H


#define TASK_COMM_LEN 16

struct event {
    bool exit_event;      // true表示退出事件，false表示执行事件
    pid_t pid;           // 进程ID
    pid_t ppid;          // 父进程ID
    char comm[TASK_COMM_LEN]; // 进程名
    int start_ts;        // 进程开始时间(纳秒)
    int duration_ns;     // 进程持续时间(纳秒，仅退出事件有效)
    int exit_code;       // 退出码(仅退出事件有效)
};

#endif /* __PROCESS_MONITOR_H */