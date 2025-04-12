#!/bin/bash

# 启动3个不同的app实例，每个实例4个线程
for app_id in {1..3}; do
    ./app_worker $app_id 4 &
done

echo "all app_worker started"
echo "ps aux | grep omp_worker"
echo "pkill app_worker  # stop all workers"

wait  # 等待所有后台进程结束