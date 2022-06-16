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
#include <sys/epoll.h>

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
    std::cerr << "create socket failed!" << endl;
    exit(0);
  }

  // 2. 绑定
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(10000);
  addr.sin_addr.s_addr = INADDR_ANY;
  int ret = bind(lfd, (struct sockaddr *) &addr, sizeof(addr));
  if (ret == -1) {
    std::cerr << "bind failed!" << endl;
    exit(0);
  }
  unordered_map<int, struct sockaddr_in> clientMap;

  // 3. 设置监听
  ret = listen(lfd, 128);
  if (ret == -1) {
    std::cerr << "listen failed!" << endl;
    exit(0);
  }
  // 现在只有监听的文件描述符
  // 所有的文件描述符对应读写缓冲区状态都是委托内核进行检测的epoll
  // 创建一个epoll模型
  int epfd = epoll_create(100);
  if (epfd == -1) {
    std::cerr << "epoll create failed!" << endl;
    exit(0);
  }
  // 往epoll实例中添加需要检测的节点, 现在只有监听的文件描述符
  struct epoll_event ev;
  ev.events = EPOLLIN;    // 检测lfd读读缓冲区是否有数据
  ev.data.fd = lfd;
  ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
  if (ret == -1) {
    std::cerr << "epoll add failed!" << endl;
    exit(0);
  }
  struct epoll_event evs[MAX_CONNECTION + 1];
  int size = sizeof(evs) / sizeof(struct epoll_event);
  // 持续检测
  while (1) {
    // 调用一次, 检测一次
    int num = epoll_wait(epfd, evs, size, -1);
    for (int i = 0; i < num; ++i) {
      // 取出当前的文件描述符
      int curfd = evs[i].data.fd;
      // 判断这个文件描述符是不是用于监听的
      if (curfd == lfd) {
        // 建立新的连接
        struct sockaddr_in cliaddr;
        socklen_t cliLen = sizeof(cliaddr);
        int cfd = accept(lfd, (struct sockaddr *) &cliaddr, &cliLen);
        clientMap[cfd] = cliaddr; // 记录客户端
        std::cout << getIpString(&cliaddr) << " connected." << endl;
        // 新得到的文件描述符添加到epoll模型中, 下一轮循环的时候就可以被检测了
        ev.events = EPOLLIN;    // 读缓冲区是否有数据
        ev.data.fd = cfd;
        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
        if (ret == -1) {
          std::cerr << "epoll add failed!" << endl;
          exit(0);
        }
      } else {
        // 处理通信的文件描述符
        // 接收数据
        char buf[BUFF_SIZE];
        memset(buf, 0, sizeof(buf));
        int len = recv(curfd, buf, sizeof(buf), 0);
        string client = getIpString(&clientMap[curfd]);
        if (len == 0) {
          std::cerr << client << " disconnected!" << endl;
          // 将这个文件描述符从epoll模型中删除
          epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, nullptr);
          close(curfd);
          clientMap.erase(curfd);
        } else if (len > 0) {
          std::cout << client << " say:" << buf << std::endl;
          send(curfd, buf, len, 0);
        } else {
          std::cerr << client << " read failed" << endl;
          exit(0);
        }
      }
    }
  } // end while(1)
  close(lfd);
  return 0;
}