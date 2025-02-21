// 虚拟机192.168.151.132 宿主机192.168.151.1 演示服务端
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "Using: ./demo02 服务端的端口" << endl;
        cout << "Example: ./demo02 5002" << endl;
        return -1;
    }

    // 创建客户端套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        return -1;
    }

    // 把服务端用于通信的IP和端口绑定到socket上
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 表示可以接收任意IP地址的客户端连接
    servaddr.sin_port = htons(atoi(argv[1]));
    // 绑定服务端的IP和端口
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("bind");
        close(listenfd);
        return -1;
    }

    // 把socket设置为可监听的状态
    if (listen(listenfd, 5) != 0) {
        perror("listen");
        close(listenfd);
        return -1;
    }

    // 受理客户端的连接请求
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    // int clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &len);
    int clientfd = accept(listenfd, 0, 0);
    if (clientfd == -1) {
        perror("accept");
        close(listenfd);
        return -1;
    }

    cout << "客户端已连接" << endl;
    // 和客户端通信，接收到发过来的报文后回复ok
    char buffer[1024];
    while (true) {
        int iret;
        memset(buffer, 0, sizeof(buffer));
        // 接收客户端的请求报文
        if ((iret = recv(clientfd, buffer, sizeof(buffer), 0)) <= 0) {
            perror("recv");
            break;
        }
        cout << "接收数据: " << buffer << endl;
        strcpy(buffer, "ok"); // 回应报文内容
        if (iret=send(clientfd, buffer, strlen(buffer), 0) <= 0) {
            perror("send");
            break;
        }
        cout << "发送数据: " << buffer << endl;
    }
    close(listenfd); // 关闭服务端用于监听的socket
    close(clientfd); // 关闭客户端连上来的socket
}