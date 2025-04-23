#!/usr/bin/env python3
from bcc import BPF, USDT
import argparse
import matplotlib.pyplot as plt
from collections import deque
import ctypes
import time

# �¼����ݽṹ����
class Event(ctypes.Structure):
    _fields_ = [
        ("duration_ns", ctypes.c_ulonglong),
        ("pid", ctypes.c_uint),
        ("tid", ctypes.c_uint)
    ]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", help="Path to target binary")
    parser.add_argument("function", help="Function name to trace")
    args = parser.parse_args()

    # ����BPF����
    bpf = BPF(src_file="moni.bpf.c",
              cflags=["-I/usr/include/x86_64-linux-gnu"])

    # ����uprobe
    sym_addr = BPF.get_elf_sym(args.binary, args.function)
    bpf.attach_uprobe(name=args.binary, 
                     sym=args.function,
                     fn_name="entry_probe",
                     addr=sym_addr)
    bpf.attach_uretprobe(name=args.binary,
                        sym=args.function,
                        fn_name="exit_probe",
                        addr=sym_addr)

    # ��ʼ����ͼ
    plt.ion()
    fig, ax = plt.subplots()
    max_points = 200
    durations = deque(maxlen=max_points)
    timestamps = deque(maxlen=max_points)
    line, = ax.plot([], [])
    ax.set_title(f"Function Duration: {args.function}")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Duration (ms)")

    start_time = time.time()

    def handle_event(cpu, data, size):
        event = ctypes.cast(data, ctypes.POINTER(Event)).contents
        durations.append(event.duration_ns / 1e6)
        timestamps.append(time.time() - start_time)

    # �����¼��ص�
    bpf["events"].open_perf_buffer(handle_event)

    print(f"Tracing {args.function} in {args.binary}... Ctrl-C to exit")
    try:
        while True:
            bpf.perf_buffer_poll()
            if len(timestamps) > 1:
                line.set_data(timestamps, durations)
                ax.relim()
                ax.autoscale_view()
                plt.pause(0.01)
    except KeyboardInterrupt:
        print("\nDetaching probes...")
        plt.close()

if __name__ == "__main__":
    main()