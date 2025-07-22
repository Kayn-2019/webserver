#pragma once
#define _GNU_SOURCE
#include "string.h"

struct Buffer
{
    // 指向内存的指针
    char* data;
    int capacity;
    int read_pos;
    int write_pos;
};

// 初始化
struct Buffer* buffer_init(int size);
void buffer_destroy(struct Buffer* buf);
// 扩容
void buffer_extend_room(struct Buffer* buffer, int size);
// 得到剩余的可写的内存容量
int buffer_writeable_size(struct Buffer* buffer);
// 得到剩余的可读的内存容量
int buffer_readable_size(struct Buffer* buffer);
// 写内存 1. 直接写 2. 接收套接字数据
int buffer_append_data(struct Buffer* buffer, const char* data, int size);
int buffer_append_string(struct Buffer* buffer, const char* data);
int buffer_socket_read(struct Buffer* buffer, int fd);
// 根据\r\n取出一行, 找到其在数据块中的位置, 返回该位置
char* buffer_find_crlf(struct Buffer* buffer);
// 发送数据
int buffer_send_data(struct Buffer* buffer, int socket);