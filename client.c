#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define DEST_PORT 3000      //目标地址端口号
#define DEST_IP "127.0.0.1" /*目标地址IP，这里设为本机*/
#define MAX_DATA 100        //接收到的数据最大程度

int main()
{
  int sockfd, new_fd;           /*cocket句柄和接受到连接后的句柄 */
  struct sockaddr_in dest_addr; /*目标地址信息*/
  char buf[MAX_DATA];           //储存接收数据

  sockfd = socket(AF_INET, SOCK_STREAM, 0); /*建立socket*/
  if (sockfd == -1)
  {
    printf("socket failed:%d\n", errno);
  }

  //参数意义见上面服务器端
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(DEST_PORT);
  dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);
  bzero(&(dest_addr.sin_zero), 8);

  if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1)
  {                                       //连接方法，传入句柄，目标地址和大小
    printf("connect failed:%d\n", errno); //失败时可以打印errno
  }
  else
  {
    printf("connect success\n");
    send(sockfd, "Hello World!", 12, 0);
    //recv(sockfd,buf,MAX_DATA,0);//将接收数据打入buf，参数分别是句柄，储存处，最大长度，其他信息（设为0即可）。
    printf("Received:%s\n", buf);
  }
  close(sockfd); //关闭socket
  return 0;
}