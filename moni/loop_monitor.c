// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "loop_monitor.skel.h"
#include "moni.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *fmt, va_list args)
{
    return vfprintf(stderr, fmt, args);
}

static int handle_event(void *ctx, void *data, size_t len)
{
    const struct event_t *e = data;
    printf("[cpu:%u tid:%u] %s at %.3f sec\n", e->cpu, e->tid, e->msg, e->timestamp / 1e9);
    return 0;
}

int main(int argc, char **argv)
{
    struct loop_monitor_bpf *skel;
    struct ring_buffer *rb = NULL;
    int err;
    char *binary;
    LIBBPF_OPTS(bpf_uprobe_opts, opts);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path-to-benchmark-bin>\n", argv[0]);
        return 1;
    }

    binary = realpath(argv[1], NULL);
    if (!binary) {
        perror("realpath");
        return 1;
    }

    libbpf_set_print(libbpf_print_fn);

    skel = loop_monitor_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "Failed to open and load BPF skeleton\n");
        return 1;
    }

    opts.func_name = "loop";
    opts.retprobe = false;
    skel->links.handle_loop_entry = bpf_program__attach_uprobe_opts(
        skel->progs.handle_loop_entry, -1, binary, 0, &opts);
    if (!skel->links.handle_loop_entry) {
        fprintf(stderr, "Failed to attach loop() entry probe\n");
        goto cleanup;
    }

    opts.retprobe = true;
    skel->links.handle_loop_exit = bpf_program__attach_uprobe_opts(
        skel->progs.handle_loop_exit, -1, binary, 0, &opts);
    if (!skel->links.handle_loop_exit) {
        fprintf(stderr, "Failed to attach loop() exit probe\n");
        goto cleanup;
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ringbuf\n");
        goto cleanup;
    }

    printf("Monitoring loop()... Press Ctrl+C to exit.\n");
    while (1) {
        err = ring_buffer__poll(rb, 100);
        if (err < 0) {
            fprintf(stderr, "Ringbuf polling failed: %d\n", err);
            break;
        }
    }

cleanup:
    ring_buffer__free(rb);
    loop_monitor_bpf__destroy(skel);
    free(binary);
    return -err;
}
