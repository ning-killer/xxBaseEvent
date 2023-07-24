#include <iostream>
#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // WIN32

#define CCT (+8)

int main() {
  printf("this is time test!\n");
  for (;;) {
    time_t t;
    time(&t);
    printf("since 1970-01-01 the secounds is ：%d\n", t);
    time_t t1;
    struct tm *info;
    time(&t1);
    info = localtime(&t1);
    printf("the local time and date is :%s", asctime(info));
    time_t t2;
    struct tm *info1;
    time(&t2);
    /* 获取 GMT 时间 */
    info1 = gmtime(&t2);
    printf("word time：\n");
    printf("GMT：%2d:%02d\n", (info1->tm_hour) % 24, info1->tm_min);
    printf("china ：%2d:%02d\n\n\n\n\n", (info1->tm_hour + CCT) % 24, info1->tm_min);
#ifdef WIN32
    Sleep(30000);
#else
    sleep(30);
#endif // WIN32
  }
  return(0);
}