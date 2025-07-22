#include "http_request.h"

 

#define HeaderSize 12
struct HttpRequest* http_request_init()
{
    struct HttpRequest* req = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
    http_request_reset(req);
    req->req_headers = (struct RequestHeader*)malloc(sizeof(struct RequestHeader) * HeaderSize);
    return req;
}

void http_request_reset(struct HttpRequest* req)
{
    req->cur_state = ParseReqLine;
    req->method = NULL;
    req->url = NULL;
    req->version = NULL;
    req->req_headers = NULL;
    req->req_headers_num = 0;
}

void http_request_reset_ex(struct HttpRequest* req)
{
    free(req->url);
    free(req->method);
    free(req->version);
    if (req->req_headers != NULL)
    {
        for (int i = 0; i < req->req_headers_num; ++i)
        {
            free(req->req_headers[i].key);
            free(req->req_headers[i].value);
        }
        free(req->req_headers);
    }
    http_request_reset(req);
}

void http_request_destroy(struct HttpRequest* req)
{
    if (req != NULL)
    {
        http_request_reset_ex(req);
        free(req);
    }
}

void http_request_add_header(struct HttpRequest* request, const char* key, const char* value)
{
    request->req_headers[request->req_headers_num].key = (char*)key;
    request->req_headers[request->req_headers_num].value = (char*)value;
    request->req_headers_num++;
}

char* http_request_get_header(struct HttpRequest* request, const char* key)
{
    if (request != NULL)
    {
        for (int i = 0; i < request->req_headers_num; ++i)
        {
            if (strncasecmp(request->req_headers[i].key, key, strlen(key)) == 0)
            {
                return request->req_headers[i].value;
            }
        }
    }
    return NULL;
}

const char* split_request_line(const char* start, const char* end, const char* sub, char** ptr)
{
    const char* space = end;  // 分配新内存
    if (sub != NULL)
    {
        space = memmem(start, end - start, sub, strlen(sub));
        assert(space != NULL);
    }
    int length = space - start;
    char* tmp = (char*)malloc(length + 1);
    strncpy(tmp, start, length);
    tmp[length] = '\0';
    *ptr = tmp;
    return space + 1;
}


bool parse_http_request_line(struct HttpRequest* request, struct Buffer* read_buf)
{
    // 读出请求行, 保存字符串结束地址
    const char* end = buffer_find_crlf(read_buf);
    // 保存字符串起始地址
    const char* start = read_buf->data + read_buf->read_pos;
    // 请求行总长度
    int lineSize = end - start;

    if (lineSize)
    {
        start = split_request_line(start, end, " ", &request->method);
        start = split_request_line(start, end, " ", &request->url);
        split_request_line(start, end, NULL, &request->version);
        // 为解析请求头做准备
        read_buf->read_pos += lineSize;
        read_buf->read_pos += 2;
        // 修改状态
        request->cur_state = ParseReqHeaders;
        return true;
    }
    return false;
}

bool parse_http_request_header(struct HttpRequest* request, struct Buffer* read_buf)
{
    char* end = buffer_find_crlf(read_buf);
    if (end != NULL)
    {
        char* start = read_buf->data + read_buf->read_pos;
        int lineSize = end - start;
        // 基于: 搜索字符串
        char* middle = memmem(start, lineSize, ": ", 2);
        if (middle != NULL)
        {
            char* key = malloc(middle - start + 1);
            strncpy(key, start, middle - start);
            key[middle - start] = '\0';

            char* value = malloc(end - middle - 2 + 1);
            strncpy(value, middle + 2, end - middle - 2);
            value[end - middle - 2] = '\0';

            http_request_add_header(request, key, value);
            // 移动读数据的位置
            read_buf->read_pos += lineSize;
            read_buf->read_pos += 2;
        }
        else
        {
            // 请求头被解析完了, 跳过空行
            read_buf->read_pos += 2;
            // 修改解析状态
            // 忽略 post 请求, 按照 get 请求处理
            request->cur_state = ParseReqDone;
        }
        return true;
    }
    return false;
}

bool parse_http_request(struct HttpRequest* request, struct Buffer* read_buf,
    struct HttpResponse* response, struct Buffer* send_buf, int socket)
{
    bool flag = true;
    while (request->cur_state != ParseReqDone)
    {
        switch (request->cur_state)
        {
        case ParseReqLine:
            flag = parse_http_request_line(request, read_buf);
            break;
        case ParseReqHeaders:
            flag = parse_http_request_header(request, read_buf);
            break;
        case ParseReqBody:
            break;
        default:
            break;
        }
        if (!flag)
        {
            return flag;
        }
        // 判断是否解析完毕了, 如果完毕了, 需要准备回复的数据
        if (request->cur_state == ParseReqDone)
        {
            // 1. 根据解析出的原始数据, 对客户端的请求做出处理
            process_http_request(request, response);
            // 2. 组织响应数据并发送给客户端
            http_response_prepare_msg(response, send_buf, socket);
        }
    }
    request->cur_state = ParseReqLine;   // 状态还原, 保证还能继续处理第二条及以后的请求
    return flag;
}

bool process_http_request(struct HttpRequest* request, struct HttpResponse* response)
{
    if (strcasecmp(request->method, "get") != 0)
    {
        return -1;
    }
    decode_msg(request->url, request->url);
    // 处理客户端请求的静态资源(目录或者文件)
    char* file = NULL;
    if (strcmp(request->url, "/") == 0)
    {
        file = "./";
    }
    else
    {
        file = request->url + 1;
    }
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在 -- 回复404
        //sendHeadMsg(cfd, 404, "Not Found", get_file_type(".html"), -1);
        //send_file("404.html", cfd);
        strcpy(response->file_name, "404.html");
        response->status_code = NotFound;
        strcpy(response->status_msg, "Not Found");
        // 响应头
        http_response_add_header(response, "Content-type", get_file_type(".html"));
        response->send_data_func = send_file;
        return 0;
    }

    strcpy(response->file_name, file);
    response->status_code = OK;
    strcpy(response->status_msg, "OK");
    // 判断文件类型
    if (S_ISDIR(st.st_mode))
    {
        // 把这个目录中的内容发送给客户端
        //sendHeadMsg(cfd, 200, "OK", get_file_type(".html"), -1);
        //send_dir(file, cfd);
        // 响应头
        http_response_add_header(response, "Content-type", get_file_type(".html"));
        response->send_data_func = send_dir;
    }
    else
    {
        // 把文件的内容发送给客户端
        //sendHeadMsg(cfd, 200, "OK", get_file_type(file), st.st_size);
        //send_file(file, cfd);
        // 响应头
        char tmp[12] = { 0 };
        sprintf(tmp, "%ld", st.st_size);
        http_response_add_header(response, "Content-type", get_file_type(file));
        http_response_add_header(response, "Content-length", tmp);
        response->send_data_func = send_file;
    }

    return false;
}

enum HttpRequestState httpRequestState(struct HttpRequest* request)
{
    return request->cur_state;
}

// 将字符转换为整形数
int hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decode_msg(char* to, char* from)
{
    for (; *from != '\0'; ++to, ++from)
    {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝, 赋值
            *to = *from;
        }

    }
    *to = '\0';
}

const char* get_file_type(const char* name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";	// 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

void send_dir(const char* dir_name, struct Buffer* send_buf, int cfd)
{
    char buf[4096] = { 0 };
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dir_name);
    struct dirent** namelist;
    int num = scandir(dir_name, &namelist, NULL, alphasort);
    for (int i = 0; i < num; ++i)
    {
        // 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
        char* name = namelist[i]->d_name;
        struct stat st;
        char sub_path[1024] = { 0 };
        sprintf(sub_path, "%s/%s", dir_name, name);
        stat(sub_path, &st);
        if (S_ISDIR(st.st_mode))
        {
            // a标签 <a href="">name</a>
            sprintf(buf + strlen(buf),
                "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
                name, name, st.st_size);
        }
        else
        {
            sprintf(buf + strlen(buf),
                "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                name, name, st.st_size);
        }
        // send(cfd, buf, strlen(buf), 0);
        buffer_append_string(send_buf, buf);
#ifndef MSG_SEND_AUTO
        buffer_send_data(send_buf, cfd);
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    // send(cfd, buf, strlen(buf), 0);
    buffer_append_string(send_buf, buf);
#ifndef MSG_SEND_AUTO
    buffer_send_data(send_buf, cfd);
#endif
    free(namelist);
}


void send_file(const char* file_name, struct Buffer* send_buf, int cfd)
{
    
    // 1. 打开文件
    int fd = open(file_name, O_RDONLY);
    assert(fd > 0);
#if 1
    while (1)
    {
        char buf[1024];
        int len = read(fd, buf, sizeof buf);
        if (len > 0)
        {
            // send(cfd, buf, len, 0);
            buffer_append_data(send_buf, buf, len);
#ifndef MSG_SEND_AUTO
            buffer_send_data(send_buf, cfd);
#endif
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            close(fd);
            perror("read");
        }
    }
#else
    off_t offset = 0;
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while (offset < size)
    {
        int ret = send_file(cfd, fd, &offset, size - offset);
        printf("ret value: %d\n", ret);
        if (ret == -1 && errno == EAGAIN)
        {
            printf("没数据...\n");
        }
    }
#endif
    close(fd);
}
