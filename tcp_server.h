#pragma once
#include "event_loop.h"
#include "thread_pool.h"
#include "tcp_connection.h"
#include "event_loop.h"
#include "thread_pool.h"
#include "log.h"
#include "channel.h"
#include <stdlib.h>
#include <arpa/inet.h>

struct Listener
{
    int lfd;
    unsigned short port;
};

struct TcpServer
{
    int thread_num;
    struct EventLoop* main_loop;
    struct ThreadPool* thread_pool;
    struct Listener* listener;
};

// 初始化
struct TcpServer* tcp_server_init(unsigned short port, int thread_num);
// 初始化监听
struct Listener* listener_init(unsigned short port);
// 启动服务器
void tcp_server_Run(struct TcpServer* server);