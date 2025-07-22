#include "tcp_server.h"




struct TcpServer* tcp_server_init(unsigned short port, int thread_num)
{
    struct TcpServer* tcp_server = (struct TcpServer*)malloc(sizeof(struct TcpServer));
    tcp_server->main_loop = event_loop_init();
    tcp_server->thread_num = thread_num;
    tcp_server->thread_pool = thread_pool_init(tcp_server->main_loop, thread_num);
    tcp_server->listener = listener_init(port);
    return tcp_server;
}

struct Listener* listener_init(unsigned short port)
{
    struct Listener* listener = malloc(sizeof(struct Listener));
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        return NULL;
    }
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
        return NULL;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
    if (ret == -1)
    {
        perror("bind");
        return NULL;
    }
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        return NULL;
    }
    listener->lfd = lfd;
    listener->port = port;
    return listener;
}

int accept_connection(void* arg)
{
    struct TcpServer* server = (struct TcpServer*)arg;
    int fd = accept(server->listener->lfd, NULL, NULL);
    struct EventLoop* event_loop = take_woker_event_loop(server->thread_pool);
    tcp_connection_init(fd, event_loop);
    return 0;
}

void tcp_server_Run(struct TcpServer* server)
{
    Debug("The server application has been launched.");
    thread_pool_run(server->thread_pool);
    struct Channel* channel = channel_init(server->listener->lfd, ReadEvent, accept_connection, NULL, NULL, server);
    event_loop_add_task(server->main_loop, channel, ADD);
    event_loop_run(server->main_loop);
}