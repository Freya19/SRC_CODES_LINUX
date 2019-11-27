/*借助条件变量模拟 生产者-消费者 问题*/
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

/*链表作为公享数据,需被互斥量保护*/
struct msg {   //定义一个结点
    struct msg *next;  //指针
    int num;
};
struct msg *head;  //头指针
// struct msg *mp;   //mp指针

/* 静态初始化 一个条件变量 和 一个互斥量*/
pthread_cond_t has_product = PTHREAD_COND_INITIALIZER;  // 一般用int初始化是动态初始化
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *consumer(void *p)
{
    struct msg *mp;

    for (;;) {
        pthread_mutex_lock(&lock);
        while (head == NULL) {           //头指针为空,说明没有节点   同时也是处理多个消费者的情况，所以（不） 可以为if（吗）
            pthread_cond_wait(&has_product, &lock);
        }

        mp = head;      
        head = mp->next;                //模拟消费掉一个产品  （删除结点的操作）
        pthread_mutex_unlock(&lock);

        printf("-Consume %lu---%d\n", pthread_self(), mp->num);
        free(mp);                       //释放了啥？释放的是malloc开辟的空间
        sleep(rand() % 4);
    }
}

void *producer(void *p)
{
    struct msg *mp;

    for (;;) {
        mp = malloc(sizeof(struct msg));   //创建一个空间
        mp->num = rand() % 1000 + 1;        //模拟生产一个产品  1-1000
        printf("-Produce -------------%d\n", mp->num);

        pthread_mutex_lock(&lock);
        mp->next = head;  //模拟把产品扔入箩筐（链表）中
        head = mp;      //模拟把产品扔入箩筐（链表）中 （头插法）
        pthread_mutex_unlock(&lock);  //马上解锁

        pthread_cond_signal(&has_product);  //发送信号，将等待在该条件变量上的一个线程唤醒

        sleep(rand() % 4);  //随机数种子，保证每次都是不一样的随机数
    }
}

int main(int argc, char *argv[])
{
    pthread_t pid, cid;
    srand(time(NULL));

    pthread_create(&pid, NULL, producer, NULL);

    pthread_create(&cid, NULL, consumer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);

    pthread_join(pid, NULL);
    pthread_join(cid, NULL);

    return 0;
}
