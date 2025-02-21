双向透明加速步骤：

1.修改get_org_dstaddr使得可以获取orig_src源地址
//Store the original destination address in remote_addr
//Return 0 on success, <0 on failure
static int get_org_dstaddr(int sockfd, struct sockaddr_storage *orig_dst, struct sockaddr_storage *orig_src){
    char orig_dst_str[INET6_ADDRSTRLEN];
    socklen_t addrlen = sizeof(*orig_dst);

    memset(orig_dst, 0, addrlen);

    //For UDP transparent proxying:
    //Set IP_RECVORIGDSTADDR socket option for getting the original 
    //destination of a datagram

    //Socket is bound to original destination
    if(getsockname(sockfd, (struct sockaddr*) orig_dst, &addrlen) 
            < 0){
        perror("getsockname");
        return -1;
    } else {
        if(orig_dst->ss_family == AF_INET){
            inet_ntop(AF_INET, 
                    &(((struct sockaddr_in*) orig_dst)->sin_addr),
                    orig_dst_str, INET_ADDRSTRLEN);
            fprintf(stderr, "Original destination %s\n", orig_dst_str);
        } else if(orig_dst->ss_family == AF_INET6){
            inet_ntop(AF_INET6, 
                    &(((struct sockaddr_in6*) orig_dst)->sin6_addr),
                    orig_dst_str, INET6_ADDRSTRLEN);
            fprintf(stderr, "Original destination %s\n", orig_dst_str);
        }
    }

    if(getpeername(sockfd, (struct sockaddr*) orig_src, &addrlen)
            < 0){
        perror("getpeername");
        return -1;
    } else {
        if(orig_src->ss_family == AF_INET){
            inet_ntop(AF_INET, 
                    &(((struct sockaddr_in*) orig_src)->sin_addr),
                    orig_dst_str, INET_ADDRSTRLEN);
            fprintf(stderr, "Original source %s\n", orig_dst_str);
        } else if(orig_src->ss_family == AF_INET6){
            inet_ntop(AF_INET6, 
                    &(((struct sockaddr_in6*) orig_src)->sin6_addr),
                    orig_dst_str, INET6_ADDRSTRLEN);
            fprintf(stderr, "Original source %s\n", orig_dst_str);
        }

        return 0;
    }
}

2.修改add_tcp_connection使得orig_src被传入connect_remote函数
struct sockaddr_storage orig_dst, orig_src;
if(get_org_dstaddr(local_fd, &orig_dst, &orig_src)){
    fprintf(stderr, "Could not get local address\n");
    close(local_fd);
    local_fd = 0;
    return NULL;
}

if((remote_fd = connect_remote(&orig_dst, &orig_src)) == 0){
    fprintf(stderr, "Failed to connect\n");
    close(remote_fd);
    close(local_fd);
    return NULL;
}

3.修改connect_remote函数使得可以使用透明模式和服务端建立连接，将下面这一段放到connect前

// char orig_src_str[INET6_ADDRSTRLEN];
// inet_ntop(AF_INET, 
//    &(((struct sockaddr_in*) orig_src)->sin_addr),
//    orig_src_str, INET_ADDRSTRLEN);
// fprintf(stderr, "connect_remote Original source %s\n", orig_src_str);
// fprintf(stderr, "connect_remote port = %d\n", ntohs(((struct sockaddr_in*) orig_src)->sin_port));

struct sockaddr_in clientaddr;
memset(&clientaddr, 0, sizeof(clientaddr));
clientaddr.sin_family = AF_INET;
clientaddr.sin_addr.s_addr = ((struct sockaddr_in*) orig_src)->sin_addr.s_addr;
clientaddr.sin_port = ((struct sockaddr_in*) orig_src)->sin_port;

// bind (connect_remote): Cannot assign requested address，使得可以绑定非本机地址
if(setsockopt(remote_fd, SOL_IP, IP_TRANSPARENT, &yes, sizeof(yes)) < 0){
    perror("setsockopt (IP_TRANSPARENT)");
    close(remote_fd);
    exit(EXIT_FAILURE);
}

if(bind(remote_fd, (struct sockaddr*) &clientaddr, sizeof(clientaddr)) < 0){
    perror("bind (connect_remote)");
    close(remote_fd);
    return 0;
}