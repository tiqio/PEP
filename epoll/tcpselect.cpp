// 采用Select模型实现网络通讯的服务端
// 虚拟机192.168.151.132 宿主机192.168.151.1 演示服务端
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <iostream>

using namespace std;

// 初始化服务端的监听端口
int initserver(int port);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "Using: ./tcpselect 服务端的端口" << endl;
        cout << "Example: ./tcpselect 5002" << endl;
        return -1;
    } 

    // 初始化服务端用于监听的socket
    int listensock = initserver(atoi(argv[1]));
    cout << "listensock = " << listensock << endl;
    if (listensock < 0) {
        cout << "initserver failed." << endl;
        return -1;
    }

    fd_set readfds; // 读事件的socket集合，大小为1024位的bitmap
    FD_ZERO(&readfds); // 清空bitmap
    FD_SET(listensock, &readfds); // 将listensock加入到bitmap中

    int maxfd = listensock;

    while (true) {
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        fd_set tmpfds = readfds; // 保存原始的bitmap
        int infds = select(maxfd+1, &tmpfds, NULL, NULL, 0); // 检测是否有事件发生
        if (infds < 0) { // select失败
            perror("select");
            break;
        } else if (infds == 0) { // select超时
            cout << "select timeout." << endl;
            continue;
        }

        // 如果infds>0说明有事件发生，infds存放了事件个数
        for (int eventfd = 0; eventfd <= maxfd; eventfd++) {
            // 默认位图为0，不用处理
            if (FD_ISSET(eventfd, &tmpfds) == 0) continue;

            // listensock发生事件，说明已连接队列中有新的客户端socket
            if (eventfd == listensock) {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock, (struct sockaddr*)&client, &len);
                if (clientsock < 0) {
                    perror("accept() failed");
                    continue;
                }
                cout << "accept clientsock = " << clientsock << endl;
                FD_SET(clientsock, &readfds);
                if (maxfd < clientsock) maxfd = clientsock;
            } else { // 客户端连接socket有事件，表示接收缓存中有数据可以读或者客户端已经断开连接
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                if (recv(eventfd, buffer, sizeof(buffer), 0) <= 0) {
                    // 此时客户端的连接断开
                    cout << "clientsock = " << eventfd << " over" << endl;
                    close(eventfd);
                    FD_CLR(eventfd, &readfds);
                    if (eventfd == maxfd) { // 重新计算maxfd的值
                        for (int ii = maxfd; ii > 0; ii--) {
                            if (FD_ISSET(ii, &readfds)) {
                                maxfd = ii; 
                                break;
                            }
                        }
                    }
                } else {
                    // 此时客户端有报文发过来
                    cout << "recv clientsock = " << eventfd << " buffer = " << buffer << endl;
                    // 把接受的报文回传
                    send(eventfd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return 0;
}

int initserver(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    unsigned int len = sizeof(opt);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len); // 允许在TIME_WAIT状态下的端口被重复使用
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }
    if (listen(sock, 5) < 0) {
        perror("listen");
        close(sock);
        return -1;
    }
    return sock;
}