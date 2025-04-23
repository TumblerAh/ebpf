#!/bin/bash
set -e

# 1. ����vmlinux.h����������ڣ�
[ ! -f vmlinux.h ] && bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

# 2. ����eBPF����
clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -I. -D__KERNEL__ -c loop_monitor.bpf.c -o loop_monitor.tmp.bpf.o
bpftool gen object loop_monitor.bpf.o loop_monitor.tmp.bpf.o
bpftool gen skeleton loop_monitor.bpf.o > loop_monitor.skel.h
rm loop_monitor.tmp.bpf.o

# 3. �����û��ռ����
clang -Wall -I. -D__KERNEL__ -c loop_monitor.c -o loop_monitor.o
clang -Wall loop_monitor.o -L/usr/lib64 -lbpf -lelf -lz -o loop_monitor

# 4. ��ʾ����
echo "build successed!"
echo "enter 'sudo ./loop_monitor' to run "