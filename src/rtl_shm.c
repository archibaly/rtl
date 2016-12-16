#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "rtl_shm.h"
#include "rtl_sem.h"

#define HEAP_MAGIC				0x1ea0

#define ALIGN_SIZE				4
#define ALIGN(size, align)		(((size) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(size, align)	((size) & ~((align) - 1))

#define MIN_SIZE				12
#define MIN_SIZE_ALIGNED     	ALIGN(MIN_SIZE, ALIGN_SIZE)
#define SIZEOF_STRUCT_MEM    	ALIGN(sizeof(struct rtl_heap_mem), ALIGN_SIZE)
#define SIZEOF_STRUCT_SHM		ALIGN(sizeof(rtl_shm_ctl_block_t), ALIGN_SIZE)

static void plug_holes(rtl_shm_ctl_block_t *scb, struct rtl_heap_mem *mem)
{
	struct rtl_heap_mem *nmem;
	struct rtl_heap_mem *pmem;

	/* plug hole forward */
	nmem = (struct rtl_heap_mem *)&scb->heap_ptr[mem->next];
	if (mem != nmem && nmem->used == 0 &&
		(uint8_t *)nmem != (uint8_t *)scb->heap_end) {
		/* if mem->next is unused and not end of scb->heap_ptr,
		 * combine mem and mem->next
		 */
		if (scb->lfree == nmem)
			scb->lfree = mem;
		mem->next = nmem->next;
		((struct rtl_heap_mem *)&scb->heap_ptr[nmem->next])->prev = (uint8_t *)mem - scb->heap_ptr;
	}

	/* plug hole backward */
	pmem = (struct rtl_heap_mem *)&scb->heap_ptr[mem->prev];
	if (pmem != mem && pmem->used == 0) {
		/* if mem->prev is unused, combine mem and mem->prev */
		if (scb->lfree == mem)
			scb->lfree = pmem;
		pmem->next = mem->next;
		((struct rtl_heap_mem *)&scb->heap_ptr[mem->next])->prev = (uint8_t *)pmem - scb->heap_ptr;
	}
}

static rtl_shm_ctl_block_t *heap_init(void *begin_addr, void *end_addr)
{
	rtl_shm_ctl_block_t *scb;
	struct rtl_heap_mem *mem;
	size_t mem_size_aligned;
	size_t extra_size = SIZEOF_STRUCT_SHM + 2 * SIZEOF_STRUCT_MEM;
	size_t begin_align = ALIGN((size_t)begin_addr, ALIGN_SIZE);
	size_t end_align = ALIGN_DOWN((size_t)end_addr, ALIGN_SIZE);

	/* alignment addr */
	if ((end_align > extra_size) && ((end_align - extra_size) >= begin_align)) {
		/* calculate the aligned memory size */
		mem_size_aligned = end_align - begin_align - extra_size;
	} else {
		return NULL;
	}

	scb = (rtl_shm_ctl_block_t *)begin_align;

	scb->heap_addr = begin_addr;
	scb->mem_size_aligned = mem_size_aligned;
	/* point to begin address of heap */
	scb->heap_ptr = (uint8_t *)(begin_align + SIZEOF_STRUCT_SHM);

	/* initialize the start of the heap */
	mem        = (struct rtl_heap_mem *)scb->heap_ptr;
	mem->magic = HEAP_MAGIC;
	mem->next  = scb->mem_size_aligned + SIZEOF_STRUCT_MEM;
	mem->prev  = 0;
	mem->used  = 0;

	/* initialize the end of the heap */
	scb->heap_end        = (struct rtl_heap_mem *)&scb->heap_ptr[mem->next];
	scb->heap_end->magic = HEAP_MAGIC;
	scb->heap_end->used  = 1;
	scb->heap_end->next  = scb->mem_size_aligned + SIZEOF_STRUCT_MEM;
	scb->heap_end->prev  = scb->mem_size_aligned + SIZEOF_STRUCT_MEM;

	scb->heap_sem_id = rtl_sem_init(1);
	if (scb->heap_sem_id < 0)
		return NULL;

	if (rtl_sem_set(scb->heap_sem_id, 1) < 0) {
		rtl_sem_del(scb->heap_sem_id);
		return NULL;
	}

	/* initialize the lowest-free pointer to the start of the heap */
	scb->lfree = (struct rtl_heap_mem *)scb->heap_ptr;

	return scb;
}

rtl_shm_ctl_block_t *rtl_shm_init(size_t size)
{
	int shm_id;
	uint8_t *addr;
	rtl_shm_ctl_block_t *scb;

	shm_id = shmget(IPC_PRIVATE, size, (SHM_R | SHM_W | IPC_CREAT));

	if (shm_id == -1) {
		fprintf(stderr, "shmget(%zu) failed: %s\n", size, strerror(errno));
		return NULL;
	}

	addr = shmat(shm_id, NULL, 0);

	if (shmctl(shm_id, IPC_RMID, NULL) == -1)
		fprintf(stderr, "shmctl(IPC_RMID) failed: %s\n", strerror(errno));

	if (addr == (void *)-1) {
		fprintf(stderr, "shmat() failed: %s\n", strerror(errno));
		return NULL;
	}

	scb = heap_init(addr, addr + size);
	if (!scb)
		goto err;

	return scb;

err:
	shmdt(addr);
	return NULL;
}

/* must be called by last process */
int rtl_shm_sem_del(rtl_shm_ctl_block_t *scb)
{
	return rtl_sem_del(scb->heap_sem_id);
}

int rtl_shm_mem_del(rtl_shm_ctl_block_t *scb)
{
	return shmdt(scb->heap_addr);
}

void *rtl_shm_malloc(rtl_shm_ctl_block_t *scb, size_t size)
{
	size_t ptr, ptr2;
	struct rtl_heap_mem *mem, *mem2;

	if (size == 0)
		return NULL;

	/* alignment size */
	size = ALIGN(size, ALIGN_SIZE);

	if (size > scb->mem_size_aligned) {
		return NULL;
	}

	/* every data block must be at least MIN_SIZE_ALIGNED long */
	if (size < MIN_SIZE_ALIGNED)
		size = MIN_SIZE_ALIGNED;

	/* take memory semaphore */
	rtl_sem_p(scb->heap_sem_id);

	for (ptr = (uint8_t *)scb->lfree - scb->heap_ptr;
		 ptr < scb->mem_size_aligned - size;
		 ptr = ((struct rtl_heap_mem *)&scb->heap_ptr[ptr])->next) {
		mem = (struct rtl_heap_mem *)&scb->heap_ptr[ptr];

		if ((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size) {
			/* mem is not used and at least perfect fit is possible:
			 * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

			if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED)) {
				/* (in addition to the above, we test if another struct rtl_heap_mem (SIZEOF_STRUCT_MEM) containing
				 * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
				 * -> split large block, create empty remainder,
				 * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
				 * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
				 * struct rtl_heap_mem would fit in but no data between mem2 and mem2->next
				 * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
				 *       region that couldn't hold data, but when mem->next gets freed,
				 *       the 2 regions would be combined, resulting in more free memory
				 */
				ptr2 = ptr + SIZEOF_STRUCT_MEM + size;

				/* create mem2 struct */
				mem2        = (struct rtl_heap_mem *)&scb->heap_ptr[ptr2];
				mem2->magic = HEAP_MAGIC;
				mem2->used  = 0;
				mem2->next  = mem->next;
				mem2->prev  = ptr;

				/* and insert it between mem and mem->next */
				mem->next = ptr2;
				mem->used = 1;

				if (mem2->next != scb->mem_size_aligned + SIZEOF_STRUCT_MEM) {
					((struct rtl_heap_mem *)&scb->heap_ptr[mem2->next])->prev = ptr2;
				}
			} else {
				/* (a mem2 struct does no fit into the user data space of mem and mem->next will always
				 * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
				 * take care of this).
				 * -> near fit or excact fit: do not split, no mem2 creation
				 * also can't move mem->next directly behind mem, since mem->next
				 * will always be used at this point!
				 */
				mem->used = 1;
			}
			/* set memory block magic */
			mem->magic = HEAP_MAGIC;

			if (mem == scb->lfree) {
				/* Find next free block after mem and update lowest free pointer */
				while (scb->lfree->used && scb->lfree != scb->heap_end)
					scb->lfree = (struct rtl_heap_mem *)&scb->heap_ptr[scb->lfree->next];
			}

			rtl_sem_v(scb->heap_sem_id);

			/* return the memory data except mem struct */
			return (uint8_t *)mem + SIZEOF_STRUCT_MEM;
		}
	}

	rtl_sem_v(scb->heap_sem_id);

	return NULL;
}

void *rtl_shm_realloc(rtl_shm_ctl_block_t *scb, void *rmem, size_t newsize)
{
	size_t size;
	size_t ptr, ptr2;
	struct rtl_heap_mem *mem, *mem2;
	void *nmem;

	/* alignment size */
	newsize = ALIGN(newsize, ALIGN_SIZE);
	if (newsize > scb->mem_size_aligned)
		return NULL;

	/* allocate a new memory block */
	if (rmem == NULL)
		return rtl_shm_malloc(scb, newsize);

	rtl_sem_p(scb->heap_sem_id);

	if ((uint8_t *)rmem < (uint8_t *)scb->heap_ptr ||
		(uint8_t *)rmem >= (uint8_t *)scb->heap_end) {
		/* illegal memory */
		rtl_sem_v(scb->heap_sem_id);
		return rmem;
	}

	mem = (struct rtl_heap_mem *)((uint8_t *)rmem - SIZEOF_STRUCT_MEM);

	ptr = (uint8_t *)mem - scb->heap_ptr;
	size = mem->next - ptr - SIZEOF_STRUCT_MEM;
	if (size == newsize) {
		/* the size is the same as */
		rtl_sem_v(scb->heap_sem_id);
		return rmem;
	}

	if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE < size) {
		/* split memory block */

		ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
		mem2 = (struct rtl_heap_mem *)&scb->heap_ptr[ptr2];
		mem2->magic= HEAP_MAGIC;
		mem2->used = 0;
		mem2->next = mem->next;
		mem2->prev = ptr;
		mem->next = ptr2;
		if (mem2->next != scb->mem_size_aligned + SIZEOF_STRUCT_MEM) {
			((struct rtl_heap_mem *)&scb->heap_ptr[mem2->next])->prev = ptr2;
		}

		plug_holes(scb, mem2);

		rtl_sem_v(scb->heap_sem_id);

		return rmem;
	}
	rtl_sem_v(scb->heap_sem_id);

	/* expand memory */
	nmem = rtl_shm_malloc(scb, newsize);
	if (nmem != NULL) {
		memcpy(nmem, rmem, size < newsize ? size : newsize);
		rtl_shm_free(scb, rmem);
	}

	return nmem;
}

void *rtl_shm_calloc(rtl_shm_ctl_block_t *scb, size_t count, size_t size)
{
	void *p;

	/* allocate 'count' objects of size 'size' */
	p = rtl_shm_malloc(scb, count * size);

	/* zero the memory */
	if (p)
		memset(p, 0, count * size);

	return p;
}

void rtl_shm_free(rtl_shm_ctl_block_t *scb, void *rmem)
{
	struct rtl_heap_mem *mem;

	if (rmem == NULL)
		return;

	if ((uint8_t *)rmem < (uint8_t *)scb->heap_ptr ||
		(uint8_t *)rmem >= (uint8_t *)scb->heap_end) {
		return;
	}

	/* Get the corresponding struct rtl_heap_mem ... */
	mem = (struct rtl_heap_mem *)((uint8_t *)rmem - SIZEOF_STRUCT_MEM);

	/* protect the heap from concurrent access */
	rtl_sem_p(scb->heap_sem_id);

	mem->used  = 0;
	mem->magic = HEAP_MAGIC;

	if (mem < scb->lfree) {
		/* the newly freed struct is now the lowest */
		scb->lfree = mem;
	}

	/* finally, see if prev or next are free also */
	plug_holes(scb, mem);
	rtl_sem_v(scb->heap_sem_id);
}
