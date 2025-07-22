#pragma once
#include "channel.h"
#include "event_loop.h"


struct EventLoop;
struct Dispatcher
{
    void* (*init) ();
    int (*add)(struct Channel* channel, struct EventLoop* event_loop);
    int (*remove)(struct Channel* channel, struct EventLoop* event_loop);
    int (*modify)(struct Channel* channel, struct EventLoop* event_loop);
    int (*dispatch)(struct EventLoop* event_loop, int timeout); // timeout(second)
    int (*clear)(struct EventLoop* event_loop);
};
