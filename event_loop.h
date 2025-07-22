#pragma once
#include "log.h"    
#include "dispatcher.h"
#include "channel.h"
#include "channel_map.h"
#include <stdbool.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

extern struct Dispatcher poll_dispatcher;
extern struct Dispatcher select_dispatcher;
extern struct Dispatcher epoll_dispatcher;

enum ElemType{ADD, DELETE, MODIFY};

struct ChannelElement
{
    int type;
    struct Channel* channel;
    struct ChannelElement* next;
};

struct EventLoop
{
    bool is_quit;
    struct Dispatcher* dispatcher;
    void* dispatcher_data;
    struct ChannelElement* head;
    struct ChannelElement* tail;
    struct ChannelMap* channel_map;
    pthread_t thread_id;
    char thread_name[32];
    pthread_mutex_t mutex;
    int socket_pair[2];
};

struct EventLoop* event_loop_init();
struct EventLoop* event_loop_init_ex(const char* thread_name);
int event_loop_run(struct EventLoop* event_loop);
int event_activate(struct EventLoop* event_loop, int fd, int events);
int event_loop_add_task(struct EventLoop* event_loop, struct Channel* channel, int type);
int event_loop_process_task(struct EventLoop* event_loop);
int event_loop_add(struct EventLoop* event_loop, struct Channel* channel);
int event_loop_remove(struct EventLoop* event_loop, struct Channel* channel);
int event_loop_modify(struct EventLoop* event_loop, struct Channel* channel);
int destroy_channel(struct EventLoop* event_loop, struct Channel* channel);




