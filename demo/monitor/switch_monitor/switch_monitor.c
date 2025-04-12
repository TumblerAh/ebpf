#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <bpf/libbpf.h>
#include "switch_monitor.skel.h"  // 使用自动生成的 skeleton
#include "switch_monitor.h"

static volatile bool exiting = false;

static void sig_handler(int sig) {
    exiting = true;
}

static int handle_event(void *ctx, void *data, size_t data_sz) {
    const struct sched_event *e = data;
    const struct cpu_load *cl = data;

    if (data_sz == sizeof(struct sched_event)) {
        char time_buf[64];
        time_t t = e->timestamp / 1000000000;
        struct tm *tm = localtime(&t);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm);

        switch (e->event_type) {
        case SCHED_EVENT_SWITCH_IN:
            printf("[%s.%06llu] IN  cpu=%2d tid=%6d (%s) prio=%2d vruntime=%12llu\n",
                   time_buf, (e->timestamp % 1000000000)/1000,
                   e->new_cpu, e->tid, e->comm, e->prio, e->vruntime);
            break;
        case SCHED_EVENT_SWITCH_OUT:
            printf("[%s.%06llu] OUT cpu=%2d tid=%6d (%s) runtime=%9lluns vruntime=%12llu\n",
                   time_buf, (e->timestamp % 1000000000)/1000,
                   e->prev_cpu, e->tid, e->comm, e->runtime, e->vruntime);
            break;
        case SCHED_EVENT_MIGRATE:
            printf("[%s.%06llu] MIGRATE tid=%6d (%s) from cpu=%2d to cpu=%2d\n",
                   time_buf, (e->timestamp % 1000000000)/1000,
                   e->tid, e->comm, e->prev_cpu, e->new_cpu);
            break;
        }
    } 
    // else if (data_sz == sizeof(struct cpu_load)) {
    //     printf("[CPU_LOAD] cpu=%2d load=%3u%%\n", cl->cpu_id, cl->load);
    // }

    return 0;
}

int main(int argc, char **argv) {
    struct ring_buffer *rb = NULL;
    struct switch_monitor_bpf *skel;
    int err;

    // 设置信号处理
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // 1. 打开并加载 BPF程序
    skel = switch_monitor_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }

    // 2. 加载 BPF 程序到内核
    err = switch_monitor_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load BPF skeleton: %d\n", err);
        goto cleanup;
    }

    // 3. 附加 BPF 程序到跟踪点
    err = switch_monitor_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
        goto cleanup;
    }

    // 4. 设置 ring buffer 回调
    rb = ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        err = -1;
        goto cleanup;
    }

    printf("Tracing scheduler events... Ctrl-C to stop.\n");

    // 5. 事件循环
    while (!exiting) {
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
    // 6. 清理资源（skeleton 自动管理）
    ring_buffer__free(rb);
    switch_monitor_bpf__destroy(skel);
    return err;
}