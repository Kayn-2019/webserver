#include "event_loop.h" 


void task_wake_up(struct EventLoop* event_loop)
{
    char* msg;
    int len = snprintf(NULL, 0, "wake up %s", event_loop->thread_name);  
    msg = (char *)malloc(len + 1);                              
    snprintf(msg, len + 1, "wake up %s", event_loop->thread_name);                                      
    write(event_loop->socket_pair[0], msg, sizeof(msg));
}

int read_local_msg(void *arg)
{
    struct EventLoop* event_loop = (struct EventLoop*)arg;
    char buf[256];
    read(event_loop->socket_pair[1], buf, sizeof(buf));
    return 0;
}

struct EventLoop* event_loop_init_ex(const char* thread_name)
{
    struct EventLoop* event_loop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
    event_loop->is_quit = false;
    event_loop->dispatcher = &epoll_dispatcher;
    event_loop->dispatcher_data = event_loop->dispatcher->init();

    event_loop->head = event_loop->tail = NULL;
    
    event_loop->channel_map = channel_map_init(128);

    event_loop->thread_id = pthread_self();
    strcpy(event_loop->thread_name, thread_name == NULL ? "MainThread" : thread_name);
    pthread_mutex_init(&event_loop->mutex, NULL);

    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, event_loop->socket_pair);
    if (ret==-1)
    {
        perror("sockepair");
        exit(0);
    }
    struct Channel* channel = channel_init(event_loop->socket_pair[1], ReadEvent,
    read_local_msg, NULL, NULL, event_loop);

    return event_loop;
}

struct EventLoop* event_loop_init()
{
    return event_loop_init_ex(NULL);
}

int event_loop_run(struct EventLoop* event_loop)
{
    assert(event_loop != NULL);
    struct Dispatcher* dispatcher = event_loop->dispatcher;
    while (!event_loop->is_quit)
    {
        dispatcher->dispatch(event_loop, 2);
        event_loop_process_task(event_loop);
    }
    return 0;
}

int event_activate(struct EventLoop* event_loop, int fd, int events)
{
    if (fd < 0 || event_loop == NULL) 
    {
        return -1;
    }
    struct Channel* channel = event_loop->channel_map->list[fd];
    if (events & ReadEvent && channel->readCallback)
    {
        channel->readCallback(channel->arg);
    }
    if (events & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }
    return 0;
}

int event_loop_add_task(struct EventLoop* event_loop, struct Channel* channel, int type)
{
    pthread_mutex_lock(&event_loop->mutex);
    struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    if (event_loop->head == NULL)
    {
        event_loop->head = event_loop->tail = node;
    }
    else 
    {
        event_loop->tail->next = node;
        event_loop->tail = node;
    }
    pthread_mutex_unlock(&event_loop->mutex);
    if (event_loop->thread_id == pthread_self())
    {
        event_loop_process_task(event_loop);
    }
    else
    {
        task_wake_up(event_loop);
    }
    return 0;
}

int event_loop_process_task(struct EventLoop* event_loop)
{
    pthread_mutex_lock(&event_loop->mutex);
    struct ChannelElement* head = event_loop->head;
    while (head != NULL)
    {
        struct Channel* channel = head->channel;
        if (head->type == ADD)
        {
            event_loop_add(event_loop, channel);
        }
        else if (head->type == DELETE)
        {
            event_loop_remove(event_loop, channel);
        }
        else if (head->type == MODIFY)
        {
            event_loop_modify(event_loop, channel);
        }
        struct ChannelElement* tmp = head;
        head = head->next;
        free(tmp);
    }
    event_loop->head = event_loop->tail = NULL;
    pthread_mutex_unlock(&event_loop->mutex);
    return 0;
}

int event_loop_add(struct EventLoop* event_loop, struct Channel* channel)
{
    int fd = channel->fd;
    struct ChannelMap* channel_map = event_loop->channel_map;
    if (fd >= channel_map->size)
    {
        if (!make_map_room(channel_map, fd, sizeof(struct Channel*)))
        {
            return -1;
        }
    }
    if (channel_map->list[fd] == NULL)
    {
        channel_map->list[fd] = channel;
        event_loop->dispatcher->add(channel, event_loop);
    }
    return 0;
}
int event_loop_remove(struct EventLoop* event_loop, struct Channel* channel)
{
    int fd = channel->fd;
    struct ChannelMap* channel_map = event_loop->channel_map;
    if (fd >= channel_map->size)
    {
        return -1;
    }
    int ret = event_loop->dispatcher->remove(channel, event_loop);
    return ret;
}
int event_loop_modify(struct EventLoop* event_loop, struct Channel* channel)
{
    int fd = channel->fd;
    struct ChannelMap* channel_map = event_loop->channel_map;
    if (fd >= channel_map->size)
    {
        return -1;
    }
    int ret = event_loop->dispatcher->modify(channel, event_loop);
    return ret;
}

int destroy_channel(struct EventLoop* event_loop, struct Channel* channel)
{
    event_loop->channel_map->list[channel->fd] = NULL;
    close(channel->fd);
    free(channel);
    return 0;
}