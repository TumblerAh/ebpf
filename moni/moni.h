// moni.h
#ifndef __MONI_H
#define __MONI_H

typedef __u64 u64;
typedef __u32 u32;

struct iter_event {
    u64 duration_ns;
    u32 iter;
    u32 tid;
};

struct event_t {
    u32 pid;
    u32 tid;
    u64 timestamp;
    u32 cpu;
    u32 iter;       // 仅用于迭代跟踪
    char msg[64];
};

#endif /* __MONI_H */