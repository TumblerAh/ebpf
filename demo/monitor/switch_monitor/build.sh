#!/bin/bash
set -e

# 1. 生成vmlinux.h（如果不存在）
[ ! -f vmlinux.h ] && bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

# 2. 编译eBPF程序
clang -g -O2 -target bpf -D__TARGET_ARCH_x86 -I. -D__KERNEL__ -c switch_monitor.bpf.c -o switch_monitor.tmp.bpf.o
bpftool gen object switch_monitor.bpf.o switch_monitor.tmp.bpf.o
bpftool gen skeleton switch_monitor.bpf.o > switch_monitor.skel.h
rm switch_monitor.tmp.bpf.o

# 3. 编译用户空间程序
clang -Wall -I. -D__KERNEL__ -c switch_monitor.c -o switch_monitor.o
clang -Wall switch_monitor.o -L/usr/lib64 -lbpf -lelf -lz -o switch_monitor

# 4. 提示运行
echo "build successed!"
echo "enter 'sudo ./switch_monitor' to run "