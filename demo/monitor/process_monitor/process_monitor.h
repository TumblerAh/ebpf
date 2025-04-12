#ifndef __PROCESS_MONITOR_H
#define __PROCESS_MONITOR_H


#define TASK_COMM_LEN 16

struct event {
    bool exit_event;      // true��ʾ�˳��¼���false��ʾִ���¼�
    pid_t pid;           // ����ID
    pid_t ppid;          // ������ID
    char comm[TASK_COMM_LEN]; // ������
    int start_ts;        // ���̿�ʼʱ��(����)
    int duration_ns;     // ���̳���ʱ��(���룬���˳��¼���Ч)
    int exit_code;       // �˳���(���˳��¼���Ч)
};

#endif /* __PROCESS_MONITOR_H */