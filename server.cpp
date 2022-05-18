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
  // 1.创建监听套接字
  int lfd = socket(AF_INET, SOCK_STREAM, 0); // TCP

  // 2. 绑定本地端口和IP
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY; // 任意IP（自动检测网卡IP）
  addr.sin_port = htons(10000);
  int ret = bind(lfd, (struct sockaddr *) &addr, sizeof(addr));
  // 3. 设置监听
  ret = listen(lfd, 128);
  // 4.等待客户端连接
  struct sockaddr_in c_addr;
  socklen_t c_addr_len = sizeof(c_addr);
  int cfd = accept(lfd, (struct sockaddr *) &c_addr, &c_addr_len);
  // 5.与客户端通信
  while (1) {
    // 接收数据
    char buff[1024];
    memset(buff, 0, sizeof(buff));
    int len = recv(cfd, buff, sizeof(buff), 0);
    if (len > 0) {
      std::cout << "client say:" << buff << std::endl;
      send(cfd, buff, len, 0); // 原路发送
    } else if (len == 0) {
      std::cerr << "client disconnected";
      break;
    } else {
      std::cerr << "read failed";
      break;
    }
  }
  close(cfd);
  close(lfd);
  return 0;
}