#include "tcp_connection.h"


int process_read(void* arg)
{
    struct TcpConnection* conn = (struct TcpConnection*)arg;
    // 接收数据
    int count = buffer_socket_read(conn->read_buf, conn->channel->fd);

    Debug("received http request data​: %s", conn->read_buf->data + conn->read_buf->read_pos);
    if (count > 0)
    {
        // 接收到了 http 请求, 解析http请求
        int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO
        write_event_enable(conn->channel, true);
        event_loop_add_task(conn->event_loop, conn->channel, MODIFY);
#endif
        bool flag = parse_http_request(conn->request, conn->read_buf, conn->response, conn->write_buf, socket);
        if (!flag)
        {
            // 解析失败, 回复一个简单的html
            char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            buffer_append_string(conn->write_buf, errMsg);
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 断开连接
        event_loop_add_task(conn->event_loop, conn->channel, DELETE);
#endif

    }
#ifndef MSG_SEND_AUTO
    // 断开连接
    event_loop_add_task(conn->event_loop, conn->channel, DELETE);
#endif
    return 0;
}

int process_write(void* arg)
{
    Debug("started sending data (triggered by the write event)...");
    struct TcpConnection* conn = (struct TcpConnection*)arg;
    // 发送数据
    int count = buffer_send_data(conn->write_buf, conn->channel->fd);
    if (count > 0)
    {
        // 判断数据是否被全部发送出去了
        if (buffer_readable_size(conn->write_buf) == 0)
        {
            // 1. 不再检测写事件 -- 修改channel中保存的事件
            write_event_enable(conn->channel, false);
            // 2. 修改dispatcher检测的集合 -- 添加任务节点
            event_loop_add_task(conn->event_loop, conn->channel, MODIFY);
            // 3. 删除这个节点
            event_loop_add_task(conn->event_loop, conn->channel, DELETE);
        }
    }
    return 0;
}

struct TcpConnection* tcp_connection_init(int fd, struct EventLoop* event_loop)
{
    struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
    conn->event_loop = event_loop;
    conn->read_buf = buffer_init(10240);
    conn->write_buf = buffer_init(10240);
    // http
    conn->request = http_request_init();
    conn->response = http_response_init();
    sprintf(conn->name, "connection-%d", fd);
    conn->channel = channel_init(fd, ReadEvent, process_read, process_write, tcp_connection_destroy, conn);
    event_loop_add_task(event_loop, conn->channel, ADD);
    Debug("establish a connection with the client: thread_name: %s, thread_id:%ld, connection_name: %s",
        event_loop->thread_name, event_loop->thread_id, conn->name);
    return conn;
}

int tcp_connection_destroy(void* arg)
{
    struct TcpConnection* conn = (struct TcpConnection*)arg;
    if (conn != NULL)
    {
        if (conn->read_buf && buffer_readable_size(conn->read_buf) == 0 &&
            conn->write_buf && buffer_writeable_size(conn->write_buf) == 0)
        {
            destroy_channel(conn->event_loop, conn->channel);
            buffer_destroy(conn->read_buf);
            buffer_destroy(conn->write_buf);
            http_request_destroy(conn->request);
            http_response_destroy(conn->response);
            free(conn);
        }
    }
    Debug("connection disconnected,  Release resources, game_over, tcp_connection_name: %s", conn->name);
    return 0;
}
