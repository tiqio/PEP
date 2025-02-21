// 虚拟机192.168.151.132 宿主机192.168.151.1 演示服务端
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>          
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <iostream>

using namespace std;

int initserver(int port);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "Using: ./tcpepoll 服务端的端口" << endl;
        cout << "Example: ./tcpepoll 5002" << endl;
        return -1;
    } 

    int listensock = initserver(atoi(argv[1]));
    if (listensock < 0) {
        cout << "initserver() failed." << endl;
        return -1;
    }

    // 创建epoll句柄
    int epollfd = epoll_create(1);
    if (epollfd < 0) {
        cout << "epoll_create() failed." << endl;
        return -1;
    }

    // 为服务端的listensock准备读事件
    epoll_event ev;
    ev.data.fd = listensock; // data或随着epoll_wait()返回的事件一起返回
    ev.events = EPOLLIN; // 让epoll监视listensock的读事件

    epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev); // 把需要监视的socket和事件加入epollfd中
    if (epollfd < 0) {
        cout << "epoll_ctl() failed." << endl;
        return -1;
    }
    
    epoll_event evs[10]; // 存放epoll返回的事件
    while (true) {
        // epoll_wait()会阻塞，直到有事件
        // 最后两个参数是：maxevents：每次能处理的最大事件数，timeout：毫秒，-1为无限等待，0为立即返回
        int infds = epoll_wait(epollfd, evs, 10, -1);
        if (infds < 0) {
            cout << "epoll_wait() failed." << endl;
            break;
        } else if (infds == 0) {
            cout << "epoll_wait() timeout." << endl;
            continue;
        }

        cout << "infds = " << infds << endl;
        for (int ii = 0; ii < infds; ii++) { // 遍历epoll返回的数组evs
            if (evs[ii].data.fd == listensock) { // 从前往后返回数据，因此不用遍历
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock, (struct sockaddr*)&client, &len);

                cout << "client socket = " << clientsock << endl;

                // 为新客户端准备读事件，并添加到epoll中
                ev.data.fd = clientsock;
                ev.events = EPOLLIN;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev) < 0) {
                    cout << "epoll_ctl() failed." << endl;
                    break;
                }
            } else {
                char buffer[1024] = {0};
                int ret = recv(evs[ii].data.fd, buffer, 1024, 0);
                if (ret <= 0) {
                    cout << "client socket = " << evs[ii].data.fd << " offline." << endl;
                    close(evs[ii].data.fd);
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, evs[ii].data.fd, NULL) < 0) {
                        cout << "epoll_ctl() failed." << endl;
                        break;
                    }
                } else {
                    cout << "client socket = " << evs[ii].data.fd << " recv data: " << buffer << endl;
                    send(evs[ii].data.fd, buffer, strlen(buffer), 0);
                }
            }
        }
    }
    return 0;
}

// 初始化服务端的监听端口。
int initserver(int port)
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock < 0)
    {
        perror("socket() failed"); return -1;
    }

    int opt = 1; unsigned int len = sizeof(opt);
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,len);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 )
    {
        perror("bind() failed"); close(sock); return -1;
    }

    if (listen(sock,5) != 0 )
    {
        perror("listen() failed"); close(sock); return -1;
    }

    return sock;
}