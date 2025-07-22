#include "dispatcher.h"
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include "channel.h"


#define P_MAX 1024

struct PollData
{
    int maxFd;
    struct pollfd fds[P_MAX];
};


static void* poll_init();
static int poll_add(struct Channel* channel, struct EventLoop* event_loop);
static int poll_remove(struct Channel* channel, struct EventLoop* event_loop);
static int poll_modify(struct Channel* channel, struct EventLoop* event_loop);
static int poll_dispatch(struct EventLoop* event_loop, int timeout);
static int poll_clear(struct EventLoop* event_loop);

struct Dispatcher poll_dispatcher = {
    poll_init,
    poll_add,
    poll_remove,
    poll_modify,
    poll_dispatch,
    poll_clear
};


static void* poll_init()
{
    struct PollData* data = (struct PollData*)malloc(sizeof(struct PollData));
    data->maxFd = 0;
    for (int i=0; i<P_MAX; i++)
    {
        data->fds[i].fd = -1;
        data->fds[i].events = 0;
        data->fds[i].revents = 0;
    }
    return data;
}

static int poll_add(struct Channel* channel, struct EventLoop* event_loop)
{
    struct PollData* data = (struct PollData*)event_loop->dispatcher_data;
    int events = 0;
    if (channel->events & ReadEvent) {
        events |= POLLIN;
    }
    if (channel->events & WriteEvent) {
        events |= POLLOUT;
    }
    int i=0;
    for (; i<P_MAX; i++)
    {
        if (data->fds[i].fd == -1)
        {
            data->fds[i].fd = channel->fd;
            data->fds[i].events = events;
            data->maxFd = i > data->maxFd ? i : data->maxFd;
            break;
        }
    }
    if (i >= P_MAX)
    {
        return -1;
    }
    return 0;
}

static int poll_remove(struct Channel* channel, struct EventLoop* event_loop)
{
    struct PollData* data = (struct PollData*)event_loop->dispatcher_data;
    int events = 0;
    if (channel->events & ReadEvent) {
        events |= POLLIN;
    }
    if (channel->events & WriteEvent) {
        events |= POLLOUT;
    }
    int i=0;
    for (; i<P_MAX; i++)
    {
        if (data->fds[i].fd == channel->fd)
        {
            data->fds[i].fd = -1;
            data->fds[i].events = events;
            break;
        }
    }
    if (i >= P_MAX) {
        return -1;
    }
    return 0;
}

static int poll_modify(struct Channel* channel, struct EventLoop* event_loop)
{
    struct PollData* data = (struct PollData*)event_loop->dispatcher_data;
    int i=0;
    for (; i<P_MAX; i++)
    {
        if (data->fds[i].fd == channel->fd)
        {
            data->fds[i].events = 0;
            data->fds[i].revents = 0;
            break;
        }
    }
    if (i >= P_MAX) {
        return -1;
    }
    return 0;
}

static int poll_dispatch(struct EventLoop* event_loop, int timeout)
{
    struct PollData* data = (struct PollData*)event_loop->dispatcher_data;
    int count = poll(data->fds, data->maxFd, timeout * 1000);
    if (count == -1)
    {
        perror("poll");
        exit(0);
    }
    for (int i=0; i<=data->maxFd; i++)
    {
        int fd = data->fds[i].fd;
        int events = data->fds[i].revents;
        if (fd == -1) continue;
        if (events & POLLIN) 
        {
            event_activate(event_loop, fd, ReadEvent);
        }
        if (events & POLLOUT)
        {
            event_activate(event_loop, fd, WriteEvent);
        }

    }
    return 0;
}

static int poll_clear(struct EventLoop* event_loop)
{
    struct PollData* data = (struct PollData*)event_loop->dispatcher_data;
    free(data);
    return 0;
}


