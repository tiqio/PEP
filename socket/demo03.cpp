// 用于获取接收缓冲区和发送缓冲区的大小
#include <iostream>
#include <cstdio> // perror
#include <cstring> // memset
#include <sys/types.h> // socklen_t
#include <sys/socket.h> // socket, getsockopt
#include <netinet/tcp.h> // TCP_NODELAY
#include <netinet/in.h> // IPPROTO_TCP

using namespace std;

int main() {
    int bufsize = 0;
    socklen_t len = sizeof(bufsize);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, &len);
    cout << "接收缓冲区大小: " << bufsize << endl;
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, &len);
    cout << "发送缓冲区大小: " << bufsize << endl;
    // 获取TCP是否开启了TCP_NODELAY选项
    int nodelay = 0;
    len = sizeof(nodelay);
    getsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, &len);
    cout << "TCP是否开启了TCP_NODELAY选项: " << nodelay << endl;
    int opt = 1;   
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,&opt,sizeof(opt));
    getsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, &len);
    cout << "TCP是否开启了TCP_NODELAY选项: " << nodelay << endl; // 禁用Nagle算法
    return 0;
}