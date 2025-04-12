#ifndef __SWITCH_MONITOR_H
#define __SWITCH_MONITOR_H


#define TASK_COMM_LEN 16
#define MAX_CPUS 256


enum sched_event_type{
    SCHED_EVENT_SWITCH_OUT,
    SCHED_EVENT_SWITCH_IN,
    SCHED_EVENT_MIGRATE //�ж���û�з����¼�Ǩ��
};

// �����¼����ݽṹ
struct sched_event {
    __u64 timestamp;        // �¼�ʱ���
    __u32 pid;              
    __u32 tid;              
    __u32 prev_cpu;         // ֮ǰ���е�CPU
    __u32 new_cpu;          // �·����CPU
    __u64 runtime;          // ��������ʱ��
    __u64 vruntime;         // ��������ʱ��
    __u32 event_type;       // �¼�����
    __s32 prio;             // ���ȼ�
    char comm[TASK_COMM_LEN]; // ��������
};

// CPU�������ݽṹ
struct cpu_load{
    __u64 timestamp;
    __u32 cpu_id;
    __u32 load;
};


#endif