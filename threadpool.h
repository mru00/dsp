#pragma once

#include "common.h"


extern void 
thread_pool_init(void (*function)(void*), unsigned short count);


extern void
thread_pool_start(void* args);

extern void
thread_wait_all();

extern int
thread_pool_cleanup();
