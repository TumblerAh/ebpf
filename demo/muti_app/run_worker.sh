#!/bin/bash

# ����3����ͬ��appʵ����ÿ��ʵ��4���߳�
for app_id in {1..3}; do
    ./app_worker $app_id 4 &
done

echo "all app_worker started"
echo "ps aux | grep omp_worker"
echo "pkill app_worker  # stop all workers"

wait  # �ȴ����к�̨���̽���