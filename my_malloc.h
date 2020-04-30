#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<limits.h>
#include<pthread.h>

/*
  The struct named data_block is the metadata.
  It can be seen as the node in the free list.
  size is the size of the data it allocated for.
  next points to the next free region.
  prev points to the previos free region.
*/

typedef struct _data_block data_block;
struct _data_block {
	size_t size;
	data_block* next;
	data_block* prev;
};

//The mutex we need
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//Thread Safe malloc/free: locking version
void* ts_malloc_lock(size_t size);
void ts_free_lock(void* ptr);

//Thread Safe malloc/free: non-locking version
void* ts_malloc_nolock(size_t size);
void ts_free_nolock(void* ptr);

/*
unsigned long get_data_segment_size();
unsigned long get_data_segment_free_space_size();
*/


//The function we need to apply the operation
data_block* find_best_fit(size_t required_size, data_block** head);
void combine(data_block* p, data_block** head);
data_block* extend_space_sbrk_unlock(size_t required_size);
data_block* extend_space_sbrk_lock(size_t required_size);
void free_opretaion(void* p, data_block** head);
void Remove(data_block* curr, data_block** head);
void Add(data_block* curr, data_block** head);
void split(data_block* curr, size_t required_size, data_block** head);

#endif
