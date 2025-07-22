#pragma once
#include "log.h"
#include "buffer.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// 定义状态码枚举
enum Httpstatus_code
{
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

// 定义响应的结构体
struct ResponseHeader
{
    char key[32];
    char value[128];
};

// 定义一个函数指针, 用来组织要回复给客户端的数据块
typedef void (*response_body)(const char* file_name, struct Buffer* send_buf, int socket);

// 定义结构体
struct HttpResponse
{
    // 状态行: 状态码, 状态描述
    enum Httpstatus_code status_code;
    char status_msg[128];
    char file_name[128];
    // 响应头 - 键值对
    struct ResponseHeader* headers;
    int head_num;
    response_body send_data_func;
};

// 初始化
struct HttpResponse* http_response_init();
// 销毁
void http_response_destroy(struct HttpResponse* response);
// 添加响应头
void http_response_add_header(struct HttpResponse*, const char* key, const char* value);
// 组织http响应数据
void http_response_prepare_msg(struct HttpResponse* response, struct Buffer* send_buf, int socket);