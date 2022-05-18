/**
* Copyright 2021 wss
* Created by wss on 5月,18, 2022
*/
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h> // close

int main() {
// 1.创建套接字
  int fd = socket(AF_INET, SOCK_STREAM, 0); // TCP

  // 2. 连接服务器
  struct sockaddr_in sv_addr;
  sv_addr.sin_family = AF_INET;
  inet_pton(AF_INET, "192.168.0.117", &sv_addr.sin_addr.s_addr);
  sv_addr.sin_port = htons(10000);
  int ret = connect(fd, (struct sockaddr *) &sv_addr, sizeof(sv_addr));
  if (ret == -1) {
    std::cerr << "connect failed";
    exit(0);
  }
  int number = 0;
  while (1) {
    // 发送数据
    char buff[1024];
    sprintf(buff, "hello, sever ... %d", number++);
    send(fd, buff, strlen(buff) + 1, 0);

    // 接收数据
    memset(buff, 0, sizeof(buff));
    int len = recv(fd, buff, sizeof(buff), 0);
    if (len > 0) {
      std::cout << "sever say:" << buff << std::endl;
    } else if (len == 0) {
      std::cerr << "server disconnected";
      break;
    } else {
      std::cerr << "read failed";
      break;
    }
    sleep(1);
  }
  close(fd);
  return 0;
}