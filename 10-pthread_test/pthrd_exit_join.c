#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct{
	int a;
	int b;
} exit_t; 				 // 结构体的类型

void *tfn(void *arg)
{
	exit_t *ret;		// 局部变量
	ret = malloc(sizeof(exit_t)); 

	ret->a = 100;
	ret->b = 300;
	pthread_exit((void *)ret);		// 所以这里ret不能用取地址(&)的方式传入

	return NULL; //should not be here.
}

int main(void)
{
	pthread_t tid;
	exit_t *retval;

	pthread_create(&tid, NULL, tfn, NULL);
	/*调用pthread_join可以获取线程的退出状态*/
	pthread_join(tid, (void **)&retval);   //retval是exit_t *类型的,所以先&(取地址),再强转为void**类型. 其中2为传出参数.
	printf("a = %d, b = %d \n", retval->a, retval->b);

	return 0;
}

