#include "http_response.h"


#define ResHeaderSize 16
struct HttpResponse* http_response_init()
{
    struct HttpResponse* response = (struct HttpResponse*)malloc(sizeof(struct HttpResponse));
    response->head_num = 0;
    int size = sizeof(struct ResponseHeader) * ResHeaderSize;
    response->headers = (struct ResponseHeader*)malloc(size);
    response->status_code = Unknown;
    // 初始化数组
    bzero(response->headers, size);
    bzero(response->status_msg, sizeof(response->status_msg));
    bzero(response->file_name, sizeof(response->file_name));
    // 函数指针
    response->send_data_func = NULL;

    return response;
}

void http_response_destroy(struct HttpResponse* response)
{
    if (response != NULL)
    {
        free(response->headers);
        free(response);
    }
}

void http_response_add_header(struct HttpResponse* response, const char* key, const char* value)
{
    if (response == NULL || key == NULL || value == NULL)
    {
        return;
    }
    strcpy(response->headers[response->head_num].key, key);
    strcpy(response->headers[response->head_num].value, value);
    response->head_num++;
}

void http_response_prepare_msg(struct HttpResponse* response, struct Buffer* send_buf, int socket)
{
    // 状态行
    char tmp[1024] = { 0 };
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->status_code, response->status_msg);
    buffer_append_string(send_buf, tmp);
    // 响应头
    for (int i = 0; i < response->head_num; ++i)
    {
        sprintf(tmp, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
        buffer_append_string(send_buf, tmp);
    }
    // 空行
    buffer_append_string(send_buf, "\r\n");
#ifndef MSG_SEND_AUTO
    buffer_send_data(send_buf, socket);
#endif

    // 回复的数据
    response->send_data_func(response->file_name, send_buf, socket);
}
