#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <stdlib.h>


typedef int(*handleFunc) (void* arg);

enum FDEvent
{
    Timeout = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04,
};

struct Channel
{
    int fd;
    int events;
    handleFunc readCallback;
    handleFunc writeCallback;
    handleFunc destroyCallback;
    void* arg;
};

struct Channel* channel_init(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void *arg);

void write_event_enable(struct Channel* channel, bool flag);

bool is_write_event_enable(struct Channel* channel);

