#include <stdio.h>
#include <time.h>
#include <bpf/libbpf.h>
#include "process_monitor.skel.h"
#include "process_monitor.h"


static int handle_event(void *ctx, void *data, size_t size)
{
    const struct event *e = data;
    char time_buf[64];
    time_t t;
    struct tm *tm;

    // ת������ʱ���Ϊ�ɶ���ʽ
    t = e->start_ts / 1000000000;
    tm = localtime(&t);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm);

    if (e->exit_event) {
        printf("EXIT: %s %s.%03d | PID: %-6d | PPID: %-6d | Duration: %6.2fms | Code: %d\n",
               e->comm, (e->start_ts % 1000000000) / 1000000,
               e->pid, e->ppid, e->duration_ns / 1000000.0, e->exit_code);
    } else {
        printf("EXEC: %s %s.%03d | PID: %-6d | PPID: %-6d | %s\n",
               e->comm, (e->start_ts % 1000000000) / 1000000,
               e->pid, e->ppid, e->comm);
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct process_monitor_bpf *skel;
    struct ring_buffer *rb = NULL;
    int err;

    // 1. ���غ���֤BPF����
    skel = process_monitor_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "Failed to open and load BPF skeleton\n");
        return 1;
    }

    // 2. ���Ӹ��ٵ�
    err = process_monitor_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF skeleton\n");
        goto cleanup;
    }

    // 3. ����ring buffer�ص�
    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        err = -1;
        goto cleanup;
    }

    printf("Monitoring omp_worker processes...\n");
    printf("EXEC/EXIT Name Timestamp          | PID    | PPID   | Duration/Info\n");
    printf("------------------------------------------------------------\n");

    // 4. ��ѯ�¼�
    while (true) {
        err = ring_buffer__poll(rb, 100 /* timeout_ms */);
        if (err == -EINTR) {
            err = 0;
            break;
        }
        if (err < 0) {
            fprintf(stderr, "Error polling ring buffer: %d\n", err);
            break;
        }
    }

cleanup:
    // 5. ������Դ
    ring_buffer__free(rb);
    process_monitor_bpf__destroy(skel);
    return -err;
}