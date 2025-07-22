#include "thread_pool.h"



struct ThreadPool* thread_pool_init(struct EventLoop* main_loop, int count)
{
    struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
    pool->index = 0;
    pool->main_loop = main_loop;
    pool->thread_num = count;
    pool->is_start = false;
    pool->worker_threads = (struct WorkerThread*)malloc(count * sizeof(struct WorkerThread));
    return pool;
}

void thread_pool_run(struct ThreadPool* pool)
{
    assert(pool && !pool->is_start);
    pool->is_start = true;
    for (int i=0; i < pool->thread_num; ++i)
    {
        worker_thread_init(&pool->worker_threads[i], pool->index);
        worker_thread_run(&pool->worker_threads[i]);
    }
}

struct EventLoop* take_woker_event_loop(struct ThreadPool* pool)
{
    assert(pool->is_start);
    if (pool->main_loop->thread_id != pthread_self())
    {
        exit(0);
    }
    struct EventLoop* event_loop = pool->main_loop;
    if (pool->thread_num > 0)
    {
        event_loop = pool->worker_threads[pool->index].event_loop;
        pool->index = ++pool->index % pool->thread_num;
    }
    return event_loop;
}
