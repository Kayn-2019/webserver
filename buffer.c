#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>

struct Buffer* buffer_init(int size)
{
    struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
    buffer->capacity = size;
    buffer->data = (char *)malloc(size);
    memset(buffer->data, 0, size);
    buffer->read_pos = buffer->write_pos = 0;
    return buffer;
}

void buffer_destroy(struct Buffer* buffer)
{
    if (buffer != NULL)
    {
        if (buffer->data != NULL)
        {
            free(buffer->data);
        }
    }
    free(buffer);
}

void buffer_extend_room(struct Buffer* buffer, int size)
{
    // 1. 内存够用 - 不需要扩容
    if (buffer_writeable_size(buffer) >= size)
    {
        return;
    }
    // 2. 内存需要合并才够用 - 不需要扩容
    // 剩余的可写的内存 + 已读的内存 > size
    else if (buffer->read_pos + buffer_writeable_size(buffer) >= size)
    {
        // 得到未读的内存大小
        int readable = buffer_readable_size(buffer);
        // 移动内存
        memcpy(buffer->data, buffer->data + buffer->read_pos, readable);
        // 更新位置
        buffer->read_pos = 0;
        buffer->write_pos = readable;
    }
    // 3. 内存不够用 - 扩容
    else
    {
        void* temp = realloc(buffer->data, buffer->capacity + size);
        if (temp == NULL)
        {
            return; // 失败了
        }
        memset(temp + buffer->capacity, 0, size);
        // 更新数据
        buffer->data = temp;
        buffer->capacity += size;
    }
}

int buffer_writeable_size(struct Buffer* buffer)
{
    return buffer->capacity - buffer->write_pos;
}

int buffer_readable_size(struct Buffer* buffer)
{
    return buffer->write_pos - buffer->read_pos;
}

int buffer_append_data(struct Buffer* buffer, const char* data, int size)
{
    if (buffer == NULL || data == NULL || size <= 0)
    {
        return -1;
    }
    // 扩容
    buffer_extend_room(buffer, size);
    // 数据拷贝
    memcpy(buffer->data + buffer->write_pos, data, size);
    buffer->write_pos += size;
    return 0;
}

int buffer_append_string(struct Buffer* buffer, const char* data)
{
    int size = strlen(data);
    int ret = buffer_append_data(buffer, data, size);
    return ret;
}

int buffer_socket_read(struct Buffer* buffer, int fd)
{
    // read/recv/readv
    struct iovec vec[2];
    // 初始化数组元素
    int writeable = buffer_writeable_size(buffer);
    vec[0].iov_base = buffer->data + buffer->write_pos;
    vec[0].iov_len = writeable;
    char* tmpbuf = (char*)malloc(40960);
    vec[1].iov_base = tmpbuf;
    vec[1].iov_len = 40960;
    int result = readv(fd, vec, 2);
    if (result == -1)
    {
        return -1;
    }
    else if (result <= writeable)
    {
        buffer->write_pos += result;
    }
    else
    {
        buffer->write_pos = buffer->capacity;
        buffer_append_data(buffer, tmpbuf, result - writeable);
    }
    free(tmpbuf);
    return result;
}

char* buffer_find_crlf(struct Buffer* buffer)
{
    // strstr --> 大字符串中匹配子字符串(遇到\0结束) char *strstr(const char *haystack, const char *needle);
    // memmem --> 大数据块中匹配子数据块(需要指定数据块大小)
    // void *memmem(const void *haystack, size_t haystacklen,
    //      const void* needle, size_t needlelen);
    char* ptr = memmem(buffer->data + buffer->read_pos, buffer_readable_size(buffer), "\r\n", 2);
    return ptr;
}

int buffer_send_data(struct Buffer* buffer, int socket)
{
    // 判断有无数据
    int readable = buffer_readable_size(buffer);
    if (readable > 0)
    {
        int count = send(socket, buffer->data + buffer->read_pos, readable, MSG_NOSIGNAL);
        if (count > 0)
        {
            buffer->read_pos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
