// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "moni.h"
// #include <bpf/ringbuf.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";


struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} events SEC(".maps");

__noinline int get_tid() {
    return bpf_get_current_pid_tgid() & 0xFFFFFFFF;
}

SEC("uprobe/loop")
int BPF_KPROBE(handle_loop_entry)
{
    struct event_t *e;
    e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) return 0;

    e->pid = bpf_get_current_pid_tgid() >> 32;
    e->tid = get_tid();
    e->timestamp = bpf_ktime_get_ns();
    e->cpu = bpf_get_smp_processor_id();
    e->iter = 0;
    __builtin_memcpy(e->msg, "loop() entry", 12);

    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("uretprobe/loop")
int BPF_KRETPROBE(handle_loop_exit)
{
    struct event_t *e;
    e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) return 0;

    e->pid = bpf_get_current_pid_tgid() >> 32;
    e->tid = get_tid();
    e->timestamp = bpf_ktime_get_ns();
    e->cpu = bpf_get_smp_processor_id();
    e->iter = 0;
    __builtin_memcpy(e->msg, "loop() exit", 11);

    bpf_ringbuf_submit(e, 0);
    return 0;
}
