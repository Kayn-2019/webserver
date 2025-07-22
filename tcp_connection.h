#pragma once
#include "event_loop.h"
#include "buffer.h"
#include "channel.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>


//#define MSG_SEND_AUTO

struct TcpConnection
{
    struct EventLoop* event_loop;
    struct Channel* channel;
    struct Buffer* read_buf;
    struct Buffer* write_buf;
    char name[32];
    // http 协议
    struct HttpRequest* request;
    struct HttpResponse* response;
};

// 初始化
struct TcpConnection* tcp_connection_init(int fd, struct EventLoop* event_loop);
int tcp_connection_destroy(void* conn);