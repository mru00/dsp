#pragma once

#include "common.h"
#include <semaphore.h>

static pthread_t     *thread_pool;
static semaphore_t   sem_free_threads;

void 
thread_pool_init(void *(*function)(void*), unsigned short count) {

  sem_init(&sem_free_threads, count);

  // multithreading
  thread_pool = (pthread_t*)malloc ( count * sizeof(pthread_t) );
  thread_info = (thread_info*)malloc ( count * sizeof(thread_info_t) );

  for ( j = 0; j < count; j++ )
	pthread_create( thread_pool + j, NULL, transformthread, thread_info + j );


}


extern void
thread_pool_start(void* args) {

  semaphore_lock ( & sem_free_threads);
}

extern void
thread_wait_all();

extern int
thread_pool_cleanup();
