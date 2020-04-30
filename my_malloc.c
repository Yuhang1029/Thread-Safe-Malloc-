#include "my_malloc.h"

/*
  There are three global variables in this program.
  head points to the first data_block in the free list, if there is no free region now, it points to NULL.
  There are two typed head, one if for lock version the other is for unlock version.
*/

data_block* head_lock = NULL;
__thread data_block* head_unlock = NULL;

/*
  This function is used for debugging.
*/
/*
void num() {
	int ans = 0;
	data_block* curr = head;
	while (curr != NULL) {
		ans++;
		if (ans == 1)
			printf("%zu  %zu  %zu\n", (size_t)curr, (size_t)curr->next, curr->size + sizeof(data_block));
		else
			printf("%zu  %zu  %zu\n", (size_t)curr, (size_t)curr->prev, curr->size + sizeof(data_block));
		curr = curr->next;
	}
	printf("%d\n", ans);
}
*/


/*
  This function is the algorithm of the best fit allocation.
  It will examine all of the free space information, and allocate an address from the free region
  which has the smallest number of bytes greater than or equal to the requested allocation size.
*/

data_block* find_best_fit(size_t required_size, data_block** head) {
  data_block* ans = NULL;
  data_block* curr = *head;
  size_t minimum = INT_MAX;
  while (curr) {
    if (curr->size >= required_size) {
      if (curr->size == required_size) {
	ans = curr;
	break;
      }
      if (curr->size < minimum) {
	minimum = curr->size;
	ans = curr;
      }
    }
    curr = curr->next;
  }
  return ans;
}


/*
  This function is used to to merge the newly freed region with any currently free adjacent regions.
*/

void combine(data_block* p, data_block** head) {
  if (p->next != NULL) {
    if ((char*)p + p->size + sizeof(data_block) == (char*)p->next) {
      p->size = p->size + sizeof(data_block) + p->next->size;
      Remove(p->next, head);
    }
  }
  
  if (p->prev != NULL) {
    if ((char*)p - p->prev->size - sizeof(data_block) == (char*)p->prev) {
      p->prev->size = p->prev->size + sizeof(data_block) + p->size;
      Remove(p, head);
    }
  }
}

/*
  When there is no suitable space for the allocation,
  the function will call sbrk() to assign new memory from heap.
*/

data_block* extend_space_sbrk_unlock(size_t required_size) {

  //total_space += required_size + sizeof(data_block);
  //useful_space += required_size;
  void* temp = sbrk(required_size + sizeof(data_block));
  if (temp == (void*)-1) {
    return NULL;
  }
  
  data_block* now = (data_block*)temp;
  now->size = required_size;
  now->next = NULL;
  now->prev = NULL;
  
  return now;
}

data_block* extend_space_sbrk_lock(size_t required_size) {

  //total_space += required_size + sizeof(data_block);
  //useful_space += required_size;
  pthread_mutex_lock(&lock);
  void* temp = sbrk(required_size + sizeof(data_block));
  pthread_mutex_unlock(&lock);
  if (temp == (void*)-1) {
    return NULL;
  }
  
  data_block* now = (data_block*)temp;
  now->size = required_size;
  now->next = NULL;
  now->prev = NULL;
  
  return now;
}


/*
  This function is used to free the memory of the current data_block.
*/
void free_opretaion(void* p, data_block** head) {
  if (p == NULL) {
    return;
  }
  //useful_space -= ptr->size;
  data_block* ptr = (data_block*)((char*)p - sizeof(data_block));
  Add(ptr, head);
  combine(ptr, head);
}


//Remove the node from the free list
void Remove(data_block* curr, data_block** head) {
  //Remove a node from a linkedlist can be divided into two parts.
  //The node which will be removed is the head or not.
  if (curr == *head) {
    *head = curr->next;
    curr->next = NULL;
    if (*head != NULL) {
      (*head)->prev = NULL;
    }
  }
  else {
    curr->prev->next = curr->next;
    if (curr->next != NULL) {
      curr->next->prev = curr->prev;
    }
    curr->prev = NULL;
    curr->next = NULL;
  }
}

//Add the node in the free list inorder
void Add(data_block* curr, data_block** head) {
  //Need to add in the head of the free list
  if (*head == NULL || curr < *head) {
    curr->next = *head;
    curr->prev = NULL;
    if (*head != NULL) {
      (* head)->prev = curr;
    }
    *head = curr;
  }
  
  else {
    data_block* temp = *head;
    while (temp->next != NULL && temp->next < curr) {
      temp = temp->next;
    }
    //temp is in the tail of the free list
    if (temp->next == NULL) {
      temp->next = curr;
      curr->prev = temp;
      curr->next = NULL;
    }
    //temp is in the middle of the free list
    else {
      curr->next = temp->next;
      curr->prev = temp;
      curr->next->prev = curr;
      temp->next = curr;
    }
  }
}

void split(data_block* curr, size_t required_size, data_block** head) {
  //Cannot split
  if (curr->size <= required_size + sizeof(data_block)) {
    Remove(curr, head);
  }
  //Can split
  else {
    //deal with new_ptr
    data_block* new_ptr = (data_block*)((char*)curr + required_size + sizeof(data_block));
    new_ptr->size = curr->size - required_size - sizeof(data_block);
    curr->size = required_size;
    
    new_ptr->prev = NULL;
    new_ptr->next = NULL;
    Remove(curr, head);
    Add(new_ptr, head);
  }
}

//Lock Version
void* ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&lock);
  if (size <= 0) {
    return NULL;
  }
  
  data_block* insert = find_best_fit(size, &head_lock);
  if (insert != NULL) {
    split(insert, size, &head_lock);
  }
  else {
    insert = extend_space_sbrk_unlock(size);
  }
  pthread_mutex_unlock(&lock);
  return insert + 1;
}

//Lock version
void ts_free_lock(void* ptr) {
  pthread_mutex_lock(&lock);
  free_opretaion(ptr, &head_lock);
  pthread_mutex_unlock(&lock);
}


//Nolock version
void* ts_malloc_nolock(size_t size) {
  if (size <= 0) {
    return NULL;
  }
  
  data_block* insert = find_best_fit(size, &head_unlock);
  if (insert != NULL) {
    split(insert, size, &head_unlock);
  }
  else {
    insert = extend_space_sbrk_lock(size);
  }
  return insert + 1;
}

//nolock version
void ts_free_nolock(void* ptr) {
  free_opretaion(ptr, &head_unlock);
}

