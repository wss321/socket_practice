#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <poll.h>

#define BUFF_SIZE 1024
#define MAX_CONNECTION 1024

using namespace std;

string getIpString(const struct sockaddr_in *addr) {
  char c_ip[32];
  stringstream ipnport;
  ipnport << inet_ntop(AF_INET, &addr->sin_addr.s_addr, c_ip, sizeof(c_ip))
          << ":"
          << ntohs(addr->sin_port);
  return ipnport.str();
}

int main() {
  // 1. 创建监听的fd
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd == -1) {
    std::cerr << "create socket failed" << endl;
    exit(0);
  }

  // 2. 绑定
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(10000);
  addr.sin_addr.s_addr = INADDR_ANY;
  int ret = bind(lfd, (struct sockaddr *) &addr, sizeof(addr));
  if (ret == -1) {
    std::cerr << "bind failed" << endl;
    exit(0);
  }
  unordered_map<int, struct sockaddr_in> clientMap;

  // 3. 设置监听
  ret = listen(lfd, 128);
  if (ret == -1) {
    std::cerr << "listen failed" << endl;
    exit(0);
  }
  // 4. 等待连接 -> 循环
  // 检测 -> 读缓冲区, 委托内核去处理
  // 数据初始化, 创建自定义的文件描述符集
  struct pollfd fds[MAX_CONNECTION + 1];
  // 初始化
  for (int i = 0; i < MAX_CONNECTION + 1; ++i) {
    fds[i].fd = -1;
    fds[i].events = POLLIN;
  }
  fds[0].fd = lfd; // 监听事件是读事件

  int maxfd = 0;
  while (1) {
    // 委托内核检测
    ret = poll(fds, maxfd + 1, -1); // 一直阻塞，直到检测的集合中有就绪的文件描述符（有事件产生）解除阻塞
    if (ret == -1) {
      perror("select");
      exit(0);
    }

    // 检测的度缓冲区有变化
    // 有新连接
    if (fds[0].revents & POLLIN) { // 按位与来判断
      // 接收连接请求
      struct sockaddr_in cliaddr;
      socklen_t len = sizeof(cliaddr);
      // 这个accept是不会阻塞的
      int cfd = accept(lfd, (struct sockaddr *) &cliaddr, &len);
      clientMap[cfd] = cliaddr; // 记录客户端
      std::cout << getIpString(&cliaddr) << " connected." << endl;
      // 委托内核检测cfd的读缓冲区
      int i;
      for (i = 1; i < MAX_CONNECTION + 1; ++i) {
        if (fds[i].fd == -1) {
          fds[i].fd = cfd;
          break;
        }
      }
      if (i == MAX_CONNECTION + 1) { // 满了
        std::cerr << "POLL FULL!" << endl;
        close(cfd);
      }
      maxfd = i > maxfd ? i : maxfd;
    }
    // 通信, 有客户端发送数据过来
    for (int i = 1; i <= maxfd; ++i) {
      // 如果在集合中, 说明读缓冲区有数据
      if (fds[i].revents & POLLIN) {
        char buf[BUFF_SIZE];
        int len = read(fds[i].fd, buf, sizeof(buf));
        string client = getIpString(&clientMap[fds[i].fd]);
        if (len > 0) {
          std::cout << client << " say:" << buf << std::endl;
          write(fds[i].fd, buf, strlen(buf) + 1); // 原路发送
        } else if (len == 0) {
          std::cerr << client << " disconnected" << endl;
          close(i);
          clientMap.erase(i);
        } else {
          std::cerr << client << " read failed" << endl;
        }
      }
    }
  }
  close(lfd);
  return 0;
}