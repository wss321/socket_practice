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
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <sstream>

#define MAX_CLIENT_NUM 128 // 最大同时通信的客户端数量

using namespace std;
void *working(void *arg);

// 子线程参数结构体
struct SockInfo {
  struct sockaddr_in addr; // 客户端地址
  int fd = -1; // 通信的文件描述符
};

struct SockInfo infos[MAX_CLIENT_NUM];// 全局数据，记录所有子线程的参数

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
  for (int i = 0; i < MAX_CLIENT_NUM; ++i) { // 初始化子线程参数
    bzero(&infos[i], sizeof(infos[i]));
    infos[i].fd = -1;
  }
  // 4.等待客户端连接
  socklen_t c_addr_len = sizeof(struct sockaddr_in);
  while (1) {
    // 找到未使用的参数位置
    struct SockInfo *pinfo;
    for (int i = 0; i < MAX_CLIENT_NUM; ++i) {
      if (infos[i].fd == -1) {
        pinfo = &infos[i];
        break;
      }
    }
    if (!pinfo) continue;
    // 子线程建立连接
    int cfd = accept(lfd, (struct sockaddr *) &pinfo->addr, &c_addr_len);
    pinfo->fd = cfd;
    if (cfd == -1) {
      std::cerr << "accept failed"<<endl;
      break;
    }
    // 创建子线程
    pthread_t tid;
    pthread_create(&tid, NULL, working, pinfo);
    pthread_detach(tid);
  }
  close(lfd);
  return 0;
}


void *working(void *params) {
  struct SockInfo *pinfo = (struct SockInfo *) params;
  // 5.与客户端通信
  // 打印客户端IP和端口
  char c_ip[32];
  int cfd = pinfo->fd;
  stringstream ipnport;
  ipnport << inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, c_ip, sizeof(c_ip))
          << ":"
          << ntohs(pinfo->addr.sin_port);
  string client = ipnport.str();
  std::cout << client << " connected." << endl;
  while (1) {
    // 接收数据
    char buff[1024];
    memset(buff, 0, sizeof(buff));
    int len = recv(cfd, buff, sizeof(buff), 0);
    if (len > 0) {
      std::cout << client << " say:" << buff << std::endl;
      send(cfd, buff, len, 0); // 原路发送
    } else if (len == 0) {
      std::cerr << client << " disconnected"<<endl;
      break;
    } else {
      std::cerr << client << " read failed"<<endl;
      break;
    }
  }
  close(cfd);
  pinfo->fd = -1;
  return nullptr;
}