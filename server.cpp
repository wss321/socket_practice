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

using namespace std;
int main() {
  // 1.创建监听套接字
  int lfd = socket(AF_INET, SOCK_STREAM, 0); // TCP
  if (lfd == -1) {
    std::cerr << "create socket failed"<<endl;
    exit(0);
  }

  // 2. 绑定本地端口和IP
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY; // 任意IP（自动检测网卡IP）
  addr.sin_port = htons(10000);
  int ret = bind(lfd, (struct sockaddr *) &addr, sizeof(addr));
  if (ret == -1) {
    std::cerr << "bind failed"<<endl;
    exit(0);
  }
  // 3. 设置监听
  ret = listen(lfd, 128);
  if (ret == -1) {
    std::cerr << "listen failed"<<endl;
    exit(0);
  }

  // 4.等待客户端连接
  struct sockaddr_in c_addr;
  socklen_t c_addr_len = sizeof(c_addr);
  int cfd = accept(lfd, (struct sockaddr *) &c_addr, &c_addr_len);
  if (cfd == -1) {
    std::cerr << "accept failed"<<endl;
    exit(0);
  }
  // 打印客户端IP和端口
  char c_ip[32];
  std::cout << "client "
            << inet_ntop(AF_INET, &c_addr.sin_addr.s_addr, c_ip, sizeof(c_ip))
            << ":"
            << ntohs(c_addr.sin_port) << " connected"<<endl
            << std::endl;
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
      std::cerr << "client disconnected"<<endl;
      break;
    } else {
      std::cerr << "read failed"<<endl;
      break;
    }
  }
  close(cfd);
  close(lfd);
  return 0;
}