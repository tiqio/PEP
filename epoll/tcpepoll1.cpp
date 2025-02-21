// 把socket设置为非阻塞式
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

int setnonblocking(int sockfd);

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
    cout << "listensock = " << listensock << endl;

    // 把监听的socket设置为非阻塞IO，可以在accept上不阻塞
    setnonblocking(listensock);
    
    int epollfd = epoll_create(1);
    epoll_event ev;
    ev.data.fd = listensock;
    ev.events = EPOLLIN;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev);
    epoll_event evs[10];

    while (true) {
        int infds = epoll_wait(epollfd, evs, 10, -1);
        if (infds < 0) {
            cout << "epoll_wait() failed." << endl;
            break;
        } else if (infds == 0) {
            cout << "epoll_wait() timeout." << endl;
            continue;
        }

        for (int ii = 0; ii < infds; ii++) {
            if (evs[ii].data.fd == listensock) {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock, (struct sockaddr*)&client, &len);
                if ((clientsock < 0) && (errno == EAGAIN)) break;

                cout << "client socket = " << clientsock << endl;

                setnonblocking(clientsock);
                ev.data.fd = clientsock;
                // 把读水平触发修改为写边缘触发，EPOLLET，ET-边缘触发
                ev.events = EPOLLOUT|EPOLLET;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev);
            } else {
                {
                    // 读触发事件
                    // // 如果是客户端连接的socket有事件，表示有报文发过来或者连接已断开。
                    // char buffer[1024]; // 存放从客户端读取的数据。
                    // memset(buffer,0,sizeof(buffer));
                    // if (recv(evs[ii].data.fd,buffer,sizeof(buffer),0)<=0)
                    // {
                    //     // 如果客户端的连接已断开。
                    //     printf("client(eventfd=%d) disconnected.\n",evs[ii].data.fd);
                    //     close(evs[ii].data.fd);            // 关闭客户端的socket
                    //     // 从epollfd中删除客户端的socket，如果socket被关闭了，会自动从epollfd中删除，所以，以下代码不必启用。
                    //     // epoll_ctl(epollfd,EPOLL_CTL_DEL,evs[ii].data.fd,0);     
                    // }
                    // else
                    // {
                    //     // 如果客户端有报文发过来。
                    //     printf("recv(eventfd=%d):%s\n",evs[ii].data.fd,buffer);
    
                    //     // 把接收到的报文内容原封不动的发回去。
                    //     send(evs[ii].data.fd,buffer,strlen(buffer),0);
                    // }

                    // 写触发事件，当连接建立后，可以进行写入
                    cout << "连接成功，触发了写事件" << endl; 
                    for (int ii = 0; ii < 10000000; ii++) {
                        // 发送30个字节，最后的参数为0，表示阻塞发送，MSG_DONTWAIT为非阻塞操作
                        // 在阻塞模式下，send会一致等待到数据被成功发送
                        if (send(ev.data.fd, "aaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbb", 30, 0) <= 0) {
                            if (errno == EAGAIN) { // 故意把缓冲区冲爆
                                cout << "send() failed." << endl;
                                break;
                            }
                        }
                        // 冲爆后，数据会被发送，知道可以发送，出现边缘触发，使得infds>0
                    }
                }
            }
        }
    }
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

int setnonblocking(int fd) {
    int flags;
    // 获取fd的状态
    if ((flags=fcntl(fd, F_GETFL, 0)) == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}