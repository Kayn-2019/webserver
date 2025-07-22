#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "tcp_server.h"

int main(int argc, char* argv[])
{
#if 1
    if (argc < 3)
    { 
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
#else
    unsigned short port = 10000;  
    chdir("/home/share");  
#endif
    // 启动服务器
    struct TcpServer* server = tcp_server_init(port, 4);
    tcp_server_Run(server);
     
    return 0;
}