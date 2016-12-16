#ifndef _RTL_SHM_H_
#define _RTL_SHM_H_

#include <stdint.h>

struct rtl_heap_mem {
	uint16_t magic;
	uint16_t used;
	size_t next, prev;
};

typedef struct {
	uint8_t *heap_addr;
	uint8_t *heap_ptr;
	int heap_sem_id;
	struct rtl_heap_mem *lfree;
	struct rtl_heap_mem *heap_end;
	size_t mem_size_aligned;
} rtl_shm_ctl_block_t;

rtl_shm_ctl_block_t *rtl_shm_init(size_t size);
int rtl_shm_sem_del(rtl_shm_ctl_block_t *scb);
int rtl_shm_mem_del(rtl_shm_ctl_block_t *scb);
void *rtl_shm_malloc(rtl_shm_ctl_block_t *scb, size_t size);
void *rtl_shm_realloc(rtl_shm_ctl_block_t *scb, void *rmem, size_t newsize);
void *rtl_shm_calloc(rtl_shm_ctl_block_t *scb, size_t count, size_t size);
void rtl_shm_free(rtl_shm_ctl_block_t *scb, void *rmem);

#endif /* _RTL_SHM_H_ */
