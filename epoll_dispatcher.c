
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dispatcher.h"
#include "channel.h"
#include "event_loop.h"


#define EP_MAX 1024

static void* epoll_init();
static int epoll_add(struct Channel* channel, struct EventLoop* event_loop);
static int epoll_remove(struct Channel* channel, struct EventLoop* event_loop);
static int epoll_modify(struct Channel* channel, struct EventLoop* event_loop);
static int epoll_dispatch(struct EventLoop* event_loop, int timeout);
static int epoll_clear(struct EventLoop* event_loop);
static int epoll_control(struct Channel* channel, struct EventLoop* event_loop, int op);

struct Dispatcher epoll_dispatcher = {
    epoll_init,
    epoll_add,
    epoll_remove,
    epoll_modify,
    epoll_dispatch,
    epoll_clear
};

struct EpollData
{
    int epfd;
    struct epoll_event* events;
};

static void* epoll_init()
{
    struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
    data->epfd = epoll_create(1);
    if (data->epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    data->events = (struct epoll_event*)calloc(EP_MAX, sizeof(struct epoll_event));
    return data;
}

static int epoll_control(struct Channel* channel, struct EventLoop* event_loop, int op)
{
    struct EpollData* data = (struct EpollData*)event_loop->dispatcher_data;
    struct epoll_event event;
    event.data.fd = channel->fd;
    event.events = 0;
    if (channel->events & ReadEvent)
    {
        event.events |= EPOLLIN;
    }
    if (channel->events & WriteEvent)
    {
        event.events |= EPOLLOUT;
    }
    int ret = epoll_ctl(data->epfd, op, channel->fd, &event);
    return ret;
}

static int epoll_add(struct Channel* channel, struct EventLoop* event_loop)
{
    int ret = epoll_control(channel, event_loop, EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_add");
        exit(0);
    }
    return ret;
}

static int epoll_remove(struct Channel* channel, struct EventLoop* event_loop)
{
    int ret = epoll_control(channel, event_loop, EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_remove");
        exit(0);
    }
    return ret;
}

static int epoll_modify(struct Channel* channel, struct EventLoop* event_loop)
{
    int ret = epoll_control(channel, event_loop, EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_modify");
        exit(0);
    }
    return ret;
}

static int epoll_dispatch(struct EventLoop* event_loop, int timeout)
{
    struct EpollData* data = (struct EpollData*)event_loop->dispatcher_data;
    int num = epoll_wait(data->epfd, data->events, EP_MAX, timeout * 1000);
    for (int i=0; i<num; i++)
    {
        int events = data->events[i].events;
        int fd = data->events[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            continue;
        }
        if (events & EPOLLIN)
        {
            event_activate(event_loop, fd, ReadEvent);
        }
        else if (events & EPOLLOUT)
        {
            event_activate(event_loop, fd, WriteEvent);
        }
    }
    return 0;
}

static int epoll_clear(struct EventLoop* event_loop)
{
    struct EpollData* data = (struct EpollData*)event_loop->dispatcher_data;
    free(data->events);
    close(data->epfd);
    free(data);
    return 0;
}


