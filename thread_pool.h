#pragma once
#include "log.h"
#include "event_loop.h"
#include "worker_thread.h"
#include "worker_thread.h"
#include "event_loop.h"
#include <stdbool.h>
#include <stdbool.h>

struct ThreadPool
{
    struct EventLoop* main_loop;
    bool is_start;
    int thread_num;
    struct WorkerThread* worker_threads;
    int index;
};

struct ThreadPool* thread_pool_init(struct EventLoop* main_loop, int count);
void thread_pool_run(struct ThreadPool* pool);
struct EventLoop* take_woker_event_loop(struct ThreadPool* pool);