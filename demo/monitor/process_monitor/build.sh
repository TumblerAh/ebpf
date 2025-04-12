#!/bin/bash
set -e

# 1. 生成vmlinux.h（如果不存在）
[ ! -f vmlinux.h ] && bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

# 2. 编译eBPF程序
clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -I. -D__KERNEL__ -c process_monitor.bpf.c -o process_monitor.tmp.bpf.o
bpftool gen object process_monitor.bpf.o process_monitor.tmp.bpf.o
bpftool gen skeleton process_monitor.bpf.o > process_monitor.skel.h
rm process_monitor.tmp.bpf.o

# 3. 编译用户空间程序
clang -Wall -I. -D__KERNEL__ -c process_monitor.c -o process_monitor.o
clang -Wall process_monitor.o -L/usr/lib64 -lbpf -lelf -lz -o process_monitor

# 4. 提示运行
echo "build successed!"
echo "enter 'sudo ./process_monitor' to run "