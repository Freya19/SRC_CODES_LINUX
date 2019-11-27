/**
 * @brief epoll基于非阻塞I/O事件驱动
 * xwp_fullstack@163.com
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_EVENTS  1024
#define BUFLEN 128
#define SERV_PORT   8080


/*
 * status:1表示在监听事件中，0表示不在 
 * last_active:记录最后一次响应时间,做超时处理
 */
struct myevent_s {
    int fd;
    int events;                                             //EPOLLIN/OUT/ERRO
    void *arg;
    void (*call_back)(int fd, int events, void *arg);       //回调函数
    int status;                                             // 表示文件描述符是否在监听的红黑树上，在 == 1；不在 == 0；
    char buf[BUFLEN];                                       //静态数组
    int len;    
    long last_active;                                       //记录每次加入红黑树的g_efd的加入时间，用于监听导致的服务器无用占用，用来踢掉没用的监听
};

int g_efd;                                                  /* epoll_create返回的句柄 ，红黑树树根*/
struct myevent_s g_events[MAX_EVENTS+1];                    /* MAX_EVENTS：红黑树能监听的最大文件描述符个数；+1：最后一个位置元素存放listen fd */

// 回调函数 1 - eventset();
void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg)
{
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    //memset(ev->buf, 0, sizeof(ev->buf));
    //ev->len = 0;
    ev->last_active = time(NULL);

    return;
}

// 函数声明
void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

//回调函数2 - eventadd(); 
void eventadd(int efd, int events, struct myevent_s *ev)
{
    struct epoll_event epv = {0, {0}};                  // 结构体 数组？
    int op;
    epv.data.ptr = ev;
    epv.events = ev->events = events;

    if (ev->status == 1) {                      // 在树上
        op = EPOLL_CTL_MOD;                     //修改
    } 
    else {                                      //不在树上
        op = EPOLL_CTL_ADD;                     //添加上树
        ev->status = 1;
    }

    if (epoll_ctl(efd, op, ev->fd, &epv) < 0)
        printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
    else
        printf("event add OK [fd=%d], op=%d, events[%0X]\n", ev->fd, op, events);

    return;
}

// 回调函数
void eventdel(int efd, struct myevent_s *ev)
{
    struct epoll_event epv = {0, {0}};

    if (ev->status != 1)
        return;

    epv.data.ptr = ev;
    ev->status = 0;
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);

    return;
}

// 回调函数
void acceptconn(int lfd, int events, void *arg)
{
    struct sockaddr_in cin;
    socklen_t len = sizeof(cin);
    int cfd, i;

    if ((cfd = accept(lfd, (struct sockaddr *)&cin, &len)) == -1) {
        if (errno != EAGAIN && errno != EINTR) {
            /* 暂时不做出错处理 */
        }
        printf("%s: accept, %s\n", __func__, strerror(errno));                  //宏：__func__，函数名
        return;
    }

    do {
        for (i = 0; i < MAX_EVENTS; i++) {
            if (g_events[i].status == 0)
                break;
        }

        if (i == MAX_EVENTS) {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }

        int flag = 0;
        if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0)
        {
            printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
            break;
        }

        eventset(&g_events[i], cfd, recvdata, &g_events[i]);                    // 对cfd进行读 -- recvdata
        eventadd(g_efd, EPOLLIN, &g_events[i]);                                 // 往树上加cfd
    } while(0);                                                                 //while(0);是不循环的，相当于goto语句，替代goto

    printf("new connect [%s:%d][time:%ld], pos[%d]\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i);

    return;
}

//  回调函数
void recvdata(int fd, int events, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;

    len = recv(fd, ev->buf, sizeof(ev->buf), 0);                                // 摘节点 ———————————————— ？？？？？？？
    eventdel(g_efd, ev);

    if (len > 0) {                                                              // 读到数据
        ev->len = len;
        ev->buf[len] = '\0';
        printf("C[%d]:%s\n", fd, ev->buf);                                      // “反射服务器”
        /* 转换为发送事件 */
        eventset(ev, fd, senddata, ev);                                         // 同epoll_ctl，回调senddata。
        eventadd(g_efd, EPOLLOUT, ev);                                          // 再挂到红黑树上
    }
    else if (len == 0) {
        close(ev->fd);
        /* ev-g_events 地址相减得到偏移元素位置 */
        printf("[fd=%d] pos[%d], closed\n", fd, ev - g_events);
    }
    else {
        close(ev->fd);
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }

    return;
}

//回调函数
void senddata(int fd, int events, void *arg)
{
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;

    len = send(fd, ev->buf, ev->len, 0);
    /*
    printf("fd=%d\tev->buf=%s\ttev->len=%d\n", fd, ev->buf, ev->len);
    printf("send len = %d\n", len);
    */

    if (len > 0) {
        printf("send[fd=%d], [%d]%s\n", fd, len, ev->buf);
        eventdel(g_efd, ev);                                                    // 从红黑树上摘
        eventset(ev, fd, recvdata, ev);                                         // 回调recvdata
        eventadd(g_efd, EPOLLIN, ev);                                           // 再加到红黑树上，并改为“读”事件
    }
    else {
        close(ev->fd);
        eventdel(g_efd, ev);
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
    }

    return;
}

void initlistensocket(int efd, short port)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);                                  // ******
    fcntl(lfd, F_SETFL, O_NONBLOCK);                                            // 更改socket的属性：非阻塞

    eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);    // 将listenfd 放到数组最后；acceptconn 是一个回调函数；&g_events[MAX_EVENTS] 是给回调函数传的参数
    eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);                              // 将lfd加入到红黑树的根节点上

    struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	bind(lfd, (struct sockaddr *)&sin, sizeof(sin));                            // ******

	listen(lfd, 20);                                                            // ******

    return;
}

int main(int argc, char *argv[])
{
    unsigned short port = SERV_PORT;                                        //定义的端口

    if (argc == 2)                                                          //argc表示定义的命令行参数，如果命令行参数传递了，就取传入的值；否则取上面定义的port
        port = atoi(argv[1]);

    g_efd = epoll_create(MAX_EVENTS+1);

    if (g_efd <= 0)
        printf("create efd in %s err %s\n", __func__, strerror(errno));

    initlistensocket(g_efd, port);                                          //初始化红黑树上所有的监听事件，传入根节点等

    /* 事件循环 */
    struct epoll_event events[MAX_EVENTS+1];                                //对应epoll_wait传出参数

	printf("server running:port[%d]\n", port);
    int checkpos = 0, i;
    while (1) {
        /* 超时验证，每次测试100个链接，不测试listenfd 当客户端60秒内没有和服务器通信，则关闭此客户端链接 */
        long now = time(NULL);
        for (i = 0; i < 100; i++, checkpos++) {
            if (checkpos == MAX_EVENTS)
                checkpos = 0;
            if (g_events[checkpos].status != 1)
                continue;
            long duration = now - g_events[checkpos].last_active;
            if (duration >= 60) {
                close(g_events[checkpos].fd);
                printf("[fd=%d] timeout\n", g_events[checkpos].fd);
                eventdel(g_efd, &g_events[checkpos]);
            }
        }

        /* 等待事件发生 */
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, 1000);            //开始监听，有listenfd时，就要回调了，回调aacceptconn
        if (nfd < 0) {
            printf("epoll_wait error, exit\n");
            break;
        }
        for (i = 0; i < nfd; i++) {                                         //
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;  //events[i]的 data 的 ptr 指针赋值给ev结构体指针
            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }

    /* 退出前释放所有资源 */
    return 0;
}


/* epoll ————>  服务器 ————>  监听 ————>  cfd ————> 可读 ————>  epoll返回 ————> read ————> cfd从树上摘下来 ————> 
    设置监听cfd事件，操作 ————> 大写转小写 ————> 等待epoll_wait 返回 ————> 回写客户端  
     ————>   cfd从树上摘下 ————>  设置监听cfd事件，操作 ————> epoll继续监听  */