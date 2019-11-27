//#include <stdio.h>
//#include <unistd.h>
//#include <stdlib.h>
//
//int main(void)
//{
//    pid_t pid;
//
//    pid = fork();
//    if (pid == -1 ) {
//        perror("fork");
//        exit(1);
//    } else if (pid > 0) {
//
//        while (1) {
//            printf("I'm parent pid = %d, parentID = %d\n", getpid(), getppid());
//            sleep(1);
//        }
//    } else if (pid == 0) {
//        while (1) {
//            printf("child  pid = %d, parentID=%d\n", getpid(), getppid());
//            sleep(1);
//        }
//    }
//
//    return 0;
//}
/*以后写代码 先写inc然后按tab键*/
/*自己试一下*/
#include <iostream>
using namespace std;
int main() {

	system("pause");
	return 0;
}
