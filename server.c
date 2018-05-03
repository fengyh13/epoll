#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define PORT 3000          //端口号
#define BACKLOG 5          /*最大监听数*/
#define MAX_EVENT_NUM 1024 //epoll事件最大数量
#define BUFFER_SIZE 10
#define true 1
#define false 0

void addFd(int epollFd, int listenFd, int enableEt)
{
  struct epoll_event event;
  event.data.fd = listenFd;
  event.events = EPOLLIN;
  if (enableEt)
  {
    event.events |= EPOLLET;
  }
  epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &event);
}

void lt(struct epoll_event *events, int num, int epollFd, int listenFd)
{
  int i = 0;
  char buf[BUFFER_SIZE];
  for (i = 0; i < num; i++)
  {
    int sockfd = events[i].data.fd;
    if (sockfd == listenFd)
    {
      struct sockaddr_in clientaddr;
      socklen_t clilen = sizeof(clientaddr);
      int connfd = accept(listenFd, (struct sockaddr *)&clientaddr, &clilen);
      addFd(epollFd, connfd, false); //对connfd使用默认的lt模式
    }
    else if (events[i].events & EPOLLIN)
    { //只要socket读缓存中还有未读的数据，这段代码就会触发
      printf("event trigger once\n");
      memset(buf, '\0', BUFFER_SIZE);
      int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
      if (ret <= 0)
      {
        close(sockfd);
        continue;
      }
      printf("get %d bytes of content:%s\n", ret, buf);
    }
    else
    {
      printf("something else happened\n");
    }
  }
}

void et(struct epoll_event *event, int num, int epollFd, int listenFd)
{
  int i = 0;
  char buf[BUFFER_SIZE];
  for (i = 0; i < num; i++)
  {
    int sockfd = event[i].data.fd;
    if (sockfd == listenFd)
    {
      struct sockaddr_in clientaddr;
      int clilen = sizeof(clientaddr);
      int connfd = accept(listenFd, (struct sockaddr *)&clientaddr, &clilen);
      addFd(epollFd, connfd, true); //多connfd开启ET模式
    }
    else if (event[i].events & EPOLLIN)
    {
      printf("event trigger once\n");
      while (1)
      { //这段代码不会重复触发，所以要循环读取数据
        memset(buf, '\0', BUFFER_SIZE);
        int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
        if (ret < 0)
        {
          if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
          {
            printf("read later\n");
            break;
          }
          close(sockfd);
          break;
        }
        else if (ret == 0)
        {
          close(sockfd);
        }
        else
        {
          printf("get %d bytes of content:%s\n", ret, buf);
        }
      }
    }
    else
    {
      printf("something else happened \n");
    }
  }
}

int main()
{
  int sockfd, new_fd;            /*socket句柄和建立连接后的句柄*/
  struct sockaddr_in my_addr;    /*本方地址信息结构体，下面有具体的属性赋值*/
  struct sockaddr_in their_addr; /*对方地址信息*/
  int sin_size;
  struct epoll_event events[MAX_EVENT_NUM];

  sockfd = socket(AF_INET, SOCK_STREAM, 0); //建立socket
  if (sockfd == -1)
  {
    printf("socket failed:%d\n", errno);
    return -1;
  }
  my_addr.sin_family = AF_INET;                /*该属性表示接收本机或其他机器传输*/
  my_addr.sin_port = htons(PORT);              /*端口号*/
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*IP，括号内容表示本机IP*/
  bzero(&(my_addr.sin_zero), 8);               /*将其他属性置0*/
  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0)
  { //绑定地址结构体和socket
    printf("bind error\n");
    return -1;
  }
  listen(sockfd, BACKLOG); //开启监听 ，第二个参数是最大监听数

  int epollFd = epoll_create(5);
  if (epollFd < 0)
  {
    perror('epoll_create err');
    exit(1);
  }

  addFd(epollFd, sockfd, true);

  while (1)
  {
    int ret = epoll_wait(epollFd, events, MAX_EVENT_NUM, -1);
    if (ret < 0)
    {
      printf("epoll failure\n");
      break;
    }

    //lt(events, ret, epollFd, sockfd);//lt模式
    et(events, ret, epollFd, sockfd); //et模式
  }

  close(sockfd);
  return 0;
}