#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int counter;
pthread_rwlock_t rwlock;  //定义一个全局锁

/* 3个线程不定时写同一全局资源，5个线程不定时读同一全局资源 */
void *th_write(void *arg)
{
    int t;
    int i = (int)arg;
    while (1) {
        pthread_rwlock_wrlock(&rwlock);
        t = counter;   
        usleep(1000);
        printf("=======write %d: %lu: counter=%d ++counter=%d\n", i, pthread_self(), t, ++counter);  //这里对counter进行++，来体现写操作
        pthread_rwlock_unlock(&rwlock);
        usleep(10000);
    }
    return NULL;
}

void *th_read(void *arg)
{
    int i = (int)arg;

    while (1) {
        pthread_rwlock_rdlock(&rwlock);
        printf("----------------------------read %d: %lu: %d\n", i, pthread_self(), counter);
        pthread_rwlock_unlock(&rwlock);
        usleep(2000);
    }
    return NULL;
}

int main(void)      //主控线程，这里不参与读写数据；仅创建和回收线程
{
    int i;
    pthread_t tid[8];         //数组，8个tid

    pthread_rwlock_init(&rwlock, NULL);                        //初始化那个全局锁，属性为null，表示默认

    for (i = 0; i < 3; i++)       //3个写线程
        pthread_create(&tid[i], NULL, th_write, (void *)i);    //参1 表示传出新线程的id；参3 表示写 的子线程

    for (i = 0; i < 5; i++)      //5个读线程
        pthread_create(&tid[i+3], NULL, th_read, (void *)i);

    for (i = 0; i < 8; i++)
        pthread_join(tid[i], NULL);

    pthread_rwlock_destroy(&rwlock);

    return 0;
}
