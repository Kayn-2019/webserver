#include "channel.h"

struct Channel* channel_init(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void *arg)
{
    struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
    channel->arg = arg;
    channel->fd = fd;
    channel->events = events;
    channel->readCallback = readFunc;
    channel->writeCallback = writeFunc;
    channel->destroyCallback = destroyFunc;
    return channel;
}

void write_event_enable(struct Channel* channel, bool flag)
{
    if (flag)
    {
        channel->events |= WriteEvent;
    }
    else
    {
        channel->events = channel->events & ~WriteEvent;
    }
}

bool is_write_event_enable(struct Channel* channel)
{
    return channel->events & WriteEvent;
}