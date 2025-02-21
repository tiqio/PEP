#include "_public.h"
using namespace idc;

// 代理路由参数的结构体
// 50 10.72.1.1 300 在50端口上监听并把数据包投向10.72.1.1:300
// srcport dstip dstport listensock是源端口监听的socket
struct st_route {
    int srcport; 
    char dstip[31];
    int dstport;
    int listensock;
}stroute;

vector<struct st_route> vroute;
bool loadroute(const char *infile); // 把代理路由参数从文件加载到vroute容器

int initserver(const int port);

int epollfd = 0; // 记录epoll的句柄

#define MAXSOCK 1024 // 最大连接数
int clientsocks[MAXSOCK]; // 每个socket连接对端的socket的值
int clientatime[MAXSOCK]; // 每个socket最后一次手法报文的事件
string clientbuffer[MAXSOCK]; // 每个socket发送内容的buffer

int conntodst(const char *ip,const int port); // 向目标地址和端口发起socket连接

void EXIT(int sig); // 进程退出函数

clogfile logfile;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Using: ./inetd logfile inifile" << endl;
        cout << "Sample: ./inetd /tmp/inetd.log /etc/inetd.conf" << endl;
        cout << "本程序的功能是正向代理，如果用到了1024以下的端口，则必须由root用户启动。" << endl;
        return -1;
    }

    closeioandsignal(true); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件
    if (logfile.open(argv[1]) == false) {{
        cout << "打开日志文件失败(" << argv[1] << ")" << endl;
        return -1;
    }}
    
    // 把代理路由配置文件加载到vroute容器
    if (loadroute(argv[2]) == false) return -1;
    logfile.write("加载代理路由参数成功(%d).\n", vroute.size());

    // 初始化服务端用于监听的socket，并存入容器
    for (auto &aa: vroute) {
        if ((aa.listensock = initserver(aa.srcport)) < 0) {
            logfile.write("initserver(%d) failed.\n", aa.srcport);
            continue;
        }

        // 把监听socket设置为非阻塞
        fcntl(aa.listensock, F_SETFL, fcntl(aa.listensock, F_GETFL, 0) | O_NONBLOCK);
    }

    // 当系统调用exec系列函数时，epollfd文件描述符会被自动关闭
    epollfd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev;

    // 为监听的socket准备读事件
    for (auto aa: vroute) {
        if (aa.listensock < 0) continue;

        ev.events = EPOLLIN;
        ev.data.fd = aa.listensock;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, aa.listensock, &ev);
    }

    struct epoll_event evs[10]; // 存放epoll返回的事件

    while (true) {
        int infds = epoll_wait(epollfd, evs, 10, -1);
        if (infds < 0) {
            logfile.write("epoll() failed.\n");
            EXIT(-1);
        }
        
        // 遍历evs，处理事件
        for (int ii = 0; ii < infds; ii++) {
            logfile.write("已发生事件fd=%d(%d).\n", evs[ii].data.fd, evs[ii].events);
        
            // 如果发生的事件是listensock，处理容器中的连接
            int jj = 0; 
            for (jj = 0; jj < vroute.size(); jj++) {
                // 判断发送事件的是哪个套接字
                if (evs[ii].data.fd == vroute[jj].listensock) {
                    // 从已连接队列中获取客户端连接上的socket
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int srcsock = accept(vroute[jj].listensock, (struct sockaddr *)&client, &len);
                    logfile.write("srcsock=%d\n", srcsock);
                    if (srcsock < 0) break;
                    if (srcsock >= MAXSOCK) {
                        logfile.write("连接数已超过最大值%d.\n", MAXSOCK);
                        close(srcsock);
                        break;
                    }

                    int dstsock = conntodst(vroute[jj].dstip, vroute[jj].dstport);
                    logfile.write("dstsock=%d\n", dstsock);
                    if (dstsock < 0) {
                        close(srcsock);
                        break;
                    }
                    if (dstsock >= MAXSOCK) {
                        logfile.write("连接数已超过最大值%d.\n", MAXSOCK);
                        close(srcsock);
                        close(dstsock);
                        break;
                    }

                    logfile.write("accept on port %d,client(%d,%d) ok.\n", vroute[jj].srcport, srcsock, dstsock);

                    // 为新的两个socket逐步读事件并添加到epoll中
                    ev.data.fd = srcsock; ev.events = EPOLLIN;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, srcsock, &ev);
                    ev.data.fd = dstsock; ev.events = EPOLLIN;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, dstsock, &ev);

                    // 记录socket对端
                    clientsocks[srcsock] = dstsock; clientatime[srcsock] = time(0);
                    clientsocks[dstsock] = srcsock; clientatime[dstsock] = time(0);

                    break;
                }
            }

            if (jj < vroute.size()) continue;

            // 如果是客户端连接触发的读事件，分为数据或中断
            // 要是数据，就把它放在对端socket的缓冲区里面
            if (evs[ii].events & EPOLLIN) {
                char buffer[5000];
                int buflen = 0;
                // 从通道的一段读数据
                if ((buflen = recv(evs[ii].data.fd, buffer, sizeof(buffer), 0)) <= 0) {
                    logfile.write("client(%d,%d) disconnected.\n", evs[ii].data.fd, clientsocks[evs[ii].data.fd]);
                    close(evs[ii].data.fd);
                    close(clientsocks[evs[ii].data.fd]);
                    clientsocks[clientsocks[evs[ii].data.fd]] = 0;
                    clientsocks[evs[ii].data.fd] = 0;
                    continue;
                }

                // 成功读到了buffer
                logfile.write("from %d,%d bytes\n",evs[ii].data.fd,buflen);

                // 把读取到的数据追加到对端socket的buffer中
                clientbuffer[clientsocks[evs[ii].data.fd]].append(buffer,buflen);

                // 修改对端socket的事件，增加写事件
                ev.data.fd = clientsocks[evs[ii].data.fd];
                ev.events = EPOLLIN | EPOLLOUT;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);

                // 更新通道两端socket的活动时间
                clientatime[evs[ii].data.fd] = time(0);
                clientatime[clientsocks[evs[ii].data.fd]] = time(0);
            }

            if (evs[ii].events & EPOLLOUT) {
                // 把socket缓冲区中的数据发送出去
                int writen = send(evs[ii].data.fd, clientbuffer[evs[ii].data.fd].data(), clientbuffer[evs[ii].data.fd].length(), 0);
                logfile.write("to %d,%d bytes.\n",evs[ii].data.fd,writen);
                
                // 删除clientbuffer中已经发送成功的数据
                clientbuffer[evs[ii].data.fd].erase(0, writen);

                // 如果socket缓冲区没有数据就不去关心socket的写事件
                if (clientbuffer[evs[ii].data.fd].length() == 0) {
                    ev.data.fd = evs[ii].data.fd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
                }
            }
        }
    }
}

// 初始化服务端的监听端口
int initserver(const int port)
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock < 0)
    {
        logfile.write("socket(%d) failed.\n",port); return -1;
    }

    int opt = 1; unsigned int len = sizeof(opt);
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,len);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 )
    {
        logfile.write("bind(%d) failed.\n",port); close(sock); return -1;
    }

    if (listen(sock,5) != 0 )
    {
        logfile.write("listen(%d) failed.\n",port); close(sock); return -1;
    }

    return sock;
}

// 把代理路由参数加载到vroute容器
bool loadroute(const char *inifile) {
    cifile ifile;
    ifile.open(inifile);
    if (ifile.open(inifile) == false) {
        logfile.write("打开代理路由参数文件(%s)失败。\n",inifile);
        return false;
    }

    string strbuffer; // 行文本
    ccmdstr cmdstr; // 分割文本

    while (true) {
        if (ifile.readline(strbuffer) == false) break;

        logfile.write("strbuffer=%s\n",strbuffer.c_str());

        // 删除说明文字
        auto pos = strbuffer.find("#");
        if (pos != string::npos) strbuffer.resize(pos);

        replacestr(strbuffer, "  ", " ", true);
        deletelrchr(strbuffer, ' ');

        // 拆分参数
        cmdstr.splittocmd(strbuffer, " ");
        if (cmdstr.size() != 3) continue;

        memset(&stroute, 0, sizeof(struct st_route));
        cmdstr.getvalue(0, stroute.srcport); // 配置stroute源端口
        logfile.write("stdroute.srcport=%d\n",stroute.srcport);
        cmdstr.getvalue(1, stroute.dstip); // 配置stroute目标ip
        logfile.write("stroute.dstip=%s\n",stroute.dstip);
        cmdstr.getvalue(2, stroute.dstport); // 配置stroute目标端口
        logfile.write("stroute.dstport=%d\n",stroute.dstport);

        vroute.push_back(stroute); // 存入vroute容器
    }

    return true;
}

// 向目标地址和端口发起socket连接且不进行阻塞
int conntodst(const char *ip, const int port) {
    logfile.write("conntodst(%s,%d).\n",ip,port);
    // 1.创建客户端的socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

    // 2.向服务器发起连接请求
    struct hostent* h;
    if ((h = gethostbyname(ip)) == 0) {
        close(sockfd);
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);

    // 把socket设置为非阻塞,F_GETFD获取文件描述符标志，F_GETFL获取文件状态标志（非阻塞要用这个）
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        if (errno != EINPROGRESS) {
            close(sockfd);
            logfile.write("connect(%s,%d) failed.\n",ip,port);
            return -1;
        }
    }
    logfile.write("connect(%s,%d) ok.\n",ip,port);

    return sockfd;
}

void EXIT(int sig)
{
    logfile.write("程序退出，sig=%d。\n\n",sig);

    // 关闭全部监听的socket。
    for (auto &aa:vroute)
        if (aa.listensock>0) close(aa.listensock);

    // 关闭全部客户端的socket。
    for (auto aa:clientsocks)
        if (aa>0) close(aa);

    close(epollfd);   // 关闭epoll。

    exit(0);
}