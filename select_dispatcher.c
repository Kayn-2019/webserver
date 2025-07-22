#include "dispatcher.h"
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include "channel.h"


#define S_MAX 1024

struct SelectData
{
    fd_set readSet;
    fd_set writeSet;
};


static void* select_init();
static int select_add(struct Channel* channel, struct EventLoop* event_loop);
static int select_remove(struct Channel* channel, struct EventLoop* event_loop);
static int select_modify(struct Channel* channel, struct EventLoop* event_loop);
static int select_dispatch(struct EventLoop* event_loop, int timeout);
static int select_clear(struct EventLoop* event_loop);
static void set_fd_set(struct Channel* channel, struct SelectData* data);
static void clear_fd_set(struct Channel* channel, struct SelectData* data);

struct Dispatcher select_dispatcher = {
    select_init,
    select_add,
    select_remove,
    select_modify,
    select_dispatch,
    select_clear
};


static void* select_init()
{
    struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
    FD_ZERO(&data->readSet);
    FD_ZERO(&data->writeSet);

    return data;
}

static void set_fd_set(struct Channel* channel, struct SelectData* data)
{
    if (channel->events & ReadEvent)
    {
        FD_SET(channel->fd, &data->readSet);
    }
    if (channel->events & WriteEvent)
    {
        FD_SET(channel->fd, &data->writeSet);
    }
}

static void clear_fd_set(struct Channel* channel, struct SelectData* data)
{
    if (channel->events & ReadEvent)
    {
        FD_CLR(channel->fd, &data->readSet);
    }
    if (channel->events & WriteEvent)
    {
        FD_CLR(channel->fd, &data->writeSet);
    }
}

static int select_add(struct Channel* channel, struct EventLoop* event_loop)
{
    struct SelectData* data = (struct SelectData*)event_loop->dispatcher_data;
    if (channel->fd >= S_MAX)
    {
        return -1;
    }
    set_fd_set(channel, data);
    return 0;
}

static int select_remove(struct Channel* channel, struct EventLoop* event_loop)
{
    struct SelectData* data = (struct SelectData*)event_loop->dispatcher_data;
    if (channel->fd >= S_MAX)
    {
        return -1;
    }
    clear_fd_set(channel, data);
    channel->destroyCallback(channel->arg);
    return 0;
}

static int select_modify(struct Channel* channel, struct EventLoop* event_loop)
{
    struct SelectData* data = (struct SelectData*)event_loop->dispatcher_data;
    if (channel->fd >= S_MAX)
    {
        return -1;
    }
    set_fd_set(channel, data);
    clear_fd_set(channel, data);
    return 0;
}

static int select_dispatch(struct EventLoop* event_loop, int timeout)
{
    struct SelectData* data = (struct SelectData*)event_loop->dispatcher_data;
    struct timeval val;
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = data->readSet;
    fd_set wrtmp = data->writeSet;
    int count = select(S_MAX, &rdtmp, &wrtmp, NULL, &val);
    if (count == -1)
    {
        perror("select ");
        exit(0);
    }
    for (int i=0; i<S_MAX; i++)
    {
        if (FD_ISSET(i, &rdtmp)) 
        {
            event_activate(event_loop, i, ReadEvent);
        }
        if (FD_ISSET(i, &wrtmp))
        {
            event_activate(event_loop, i, WriteEvent);
        }

    }
    return 0;
}

static int select_clear(struct EventLoop* event_loop)
{
    struct  SelectData* data = (struct SelectData*)event_loop->dispatcher_data;
    free(data);
    return 0;
}


