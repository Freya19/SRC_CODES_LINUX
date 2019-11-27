#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t m;   //定义全局互斥量,变量 m 只有两种取值: 1、0

void err_thread(int ret, char *str)
{
    if (ret != 0) {
        fprintf(stderr, "%s:%s\n", str, strerror(ret));
        pthread_exit(NULL);
    }
}

void *tfn(void *arg)        //pthread_create(&tid, NULL, tfn, NULL);中的主控函数
{
    srand(time(NULL));

    while (1) {             //一个死循环
        pthread_mutex_lock(&m);     // m--  （加锁）

        printf("hello ");
        sleep(rand() % 3);	/*模拟长时间操作共享资源，导致cpu易主，产生与时间有关的错误*/
        printf("world\n");
        pthread_mutex_unlock(&m);   // m++  （解锁）
        sleep(rand() % 3);

    }

    return NULL;
}

int main(void)
{
    pthread_t tid;    //线程id
    srand(time(NULL));
    int flag  = 5;

    pthread_mutex_init(&m, NULL);        // 1
    int ret = pthread_create(&tid, NULL, tfn, NULL);
    err_thread(ret, "pthread_create error");

    
    while (flag--) {                /*主线程输出5次后试图销毁锁，但子线程(未推出)未将锁释放，无法完成。*/
        pthread_mutex_lock(&m);     // m--

        printf("HELLO ");
        sleep(rand() % 3);
        printf("WORLD\n");
        pthread_mutex_unlock(&m);     // m++

        sleep(rand() % 3);

    }
    pthread_cancel(tid);            // main 中加pthread_cancel()将子线程取消
    pthread_join(tid, NULL);

    pthread_mutex_destroy(&m);    //这个全局互斥量定义后，使用完要销毁掉！

    return 0;
}

/*线程之间共享资源stdout*/
