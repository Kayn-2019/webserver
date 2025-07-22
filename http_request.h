#pragma once
#include "buffer.h"
#include "http_response.h"
#include "http_response.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

// 请求头键值对
struct RequestHeader
{
    char* key;
    char* value;
};

// 当前的解析状态
enum HttpRequestState
{
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};
// 定义http请求结构体
struct HttpRequest
{
    char* method;
    char* url;
    char* version;
    struct RequestHeader* req_headers;
    int req_headers_num;
    enum HttpRequestState cur_state;
};

// 初始化
struct HttpRequest* http_request_init();
// 重置
void http_request_reset(struct HttpRequest* req);
void http_request_reset_ex(struct HttpRequest* req);
void http_request_destroy(struct HttpRequest* req);
// 获取处理状态
enum HttpRequestState http_request_state(struct HttpRequest* request);
// 添加请求头
void http_request_add_header(struct HttpRequest* request, const char* key, const char* value);
// 根据key得到请求头的value
char* http_request_get_header(struct HttpRequest* request, const char* key);
// 解析请求行
bool parse_http_request_line(struct HttpRequest* request, struct Buffer* readBuf);
// 解析请求头
bool parse_http_request_header(struct HttpRequest* request, struct Buffer* readBuf);
// 解析http请求协议
bool parse_http_request(struct HttpRequest* request, struct Buffer* read_buf,
    struct HttpResponse* response, struct Buffer* send_buf, int socket);
// 处理http请求协议
bool process_http_request(struct HttpRequest* request, struct HttpResponse* response);
// 解码字符串
void decode_msg(char* to, char* from);
const char* get_file_type(const char* name);
void send_dir(const char* dir_name, struct Buffer* send_buf, int cfd);
void send_file(const char* file_name, struct Buffer* send_buf, int cfd);