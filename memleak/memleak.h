#ifndef __MEMLEAK_H
#define __MEMLEAK_H
 
#define ALLOCS_MAX_ENTRIES 1000000
#define COMBINED_ALLOCS_MAX_ENTRIES 10240
 
struct alloc_info {
    __u64 size;
    int stack_id;
};
 
/* Ϊ�˽�ʡ�ڴ�ͷ����������ݵ�ԭ�Ӳ���,�� combined_alloc_info ����Ϊ������
 * ���� total_size ռ 40bit, number_of_allocs ռ 24bit, �������ܴ�СΪ 64bit
 * 2��combined_alloc_info������� bits �ֶ����, �൱�ڶ�Ӧ�� total_size ���, 
 * �Ͷ�Ӧ�� number_of_allocs ���;
 */
union combined_alloc_info {
    struct {
        __u64 total_size : 40;
        __u64 number_of_allocs : 24;
    };
    __u64 bits;
};
 
#endif /* __MEMLEAK_H */