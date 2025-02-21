// 虚拟机192.168.151.132 宿主机192.168.151.1 演示客户端
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
    if (argc != 3) {
        cout << "Using: ./demo01 服务端的IP和端口" << endl;
        cout << "Example: ./demo01 192.168.101.139 5005" << endl;
        return -1;
    }

    // 创建客户端套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    // 向服务器发送连接请求
    struct hostent *h; // 存放服务端IP的结构体
    if ((h = gethostbyname(argv[1])) == 0) {
        cout << "gethostbyname failed." << endl;
        close(sockfd);
        return -1;
    }

    struct sockaddr_in servaddr; // 存放IP和端口的结构体
    memset(&servaddr, 0, sizeof(servaddr)); // 初始化
    servaddr.sin_family = AF_INET;
    memcpy(&servaddr.sin_addr, h->h_addr, h->h_length); // 指定服务端的IP地址
    servaddr.sin_port = htons(atoi(argv[2])); // atoi会把字符串转化为一个整数

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    // 和服务端通讯
    char buffer[1024];
    for (int ii = 0; ii < 3; ii++) {
        int iret;
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "Number %d, code %03d", ii+1, ii+1);
        // 向服务端发送请求报文
        if ((iret = send(sockfd, buffer, strlen(buffer), 0)) <= 0) {
            perror("send");
            break;
        }
        cout << "发送数据: " << buffer << endl;

        memset(buffer, 0, sizeof(buffer));
        // 接收服务端的回应报文
        if ((iret = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0) {
            perror("recv");
            break;
        }
        cout << "接收数据: " << buffer << endl;
        sleep(1);
    }     

    // 关闭套接字
    close(sockfd);
}