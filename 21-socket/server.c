#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <arpa/inet.h> //包含socket_in结构体
#include <unistd.h>
#include <ctype.h>  //toupper()函数的头文件

#define SERV_PORT 8000

int main(void)
{
    int sfd, cfd;
    int i, len;
    struct sockaddr_in serv_addr, client_addr;
    char buf[4096], client_ip[128];  //char buf[BUFSIZ]; 可以利用系统自带的宏来设置buf的大小
    socklen_t addr_len;

    //AF_INET:ipv4   SOCK_STREAM:流协议   0:默认协议(tcp,udp)
    sfd = socket(AF_INET, SOCK_STREAM, 0);

    //绑定前先构造出服务器地址
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //网络字节序
    serv_addr.sin_port = htons(SERV_PORT);
    //INADDR_ANY主机所有ip
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));        // 参2 (struct sockaddr *)&serv_addr  强转    还有一个 connect 也要强转

    //服务器能接收并发链接的能力
    listen(sfd, 128);

    printf("wait for connect ...\n");
    addr_len = sizeof(client_addr);  //accept() 参3的初始化
    //阻塞，等待客户端链接，成功则返回新的文件描述符，用于和客户端通信        // 参2 (struct sockaddr *)&serv_addr  强转
    cfd = accept(sfd, (struct sockaddr *)&client_addr, &addr_len);  //参2 传出参数（故：取地址）；参3 传入（故：初始化）传出（故：取地址）参数
    printf("client IP:%s\t%d\n", 
            inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
            ntohs(client_addr.sin_port));

    while (1) {   // 无限链接？ 不，是无限回写
        //阻塞接收客户端数据
        len = read(cfd, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, len);

        //处理业务
        for (i = 0; i < len; i++)
            buf[i] = toupper(buf[i]);    // 把小写字符转化成大写的
        //返回给客户端结果
        write(cfd, buf, len);
    }

    close(cfd);
    close(sfd);

    return 0;
}
