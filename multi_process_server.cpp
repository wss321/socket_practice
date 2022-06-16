/**
* Copyright 2021 wss
* Created by wss on 5月,19, 2022
*/
/*
 * 父进程：
  负责监听，处理客户端的连接请求，也就是在父进程中循环调用 accept() 函数
  创建子进程：建立一个新的连接，就创建一个新的子进程，让这个子进程和对应的客户端通信
  回收子进程资源：子进程退出回收其内核 PCB 资源，防止出现僵尸进程

  子进程：负责通信，基于父进程建立新连接之后得到的文件描述符，和对应的客户端完成数据的接收和发送。
  发送数据：send() / write()
  接收数据：recv() / read()
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <iostream>
#include <sstream>

#define SERV_PORT 10000
#define BUFFER_SIZE 1024

using namespace std;

string getIpString(const struct sockaddr_in *addr) {
  char c_ip[32];
  stringstream ipnport;
  ipnport << inet_ntop(AF_INET, &addr->sin_addr.s_addr, c_ip, sizeof(c_ip))
          << ":"
          << ntohs(addr->sin_port);
  return ipnport.str();
}

int working(int cfd, string &client);
// 信号处理函数
void callback(int num) {
  while (1) {
    pid_t pid = waitpid(-1, NULL, WNOHANG);
    if (pid <= 0) {
      printf("子进程正在运行, 或者子进程被回收完毕了\n");
      break;
    }
    printf("child die, pid = %d\n", pid);
  }
}

int main() {
  int lfd, cfd; // 文件描述符
  pid_t child_pid; // 子进程ID
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;
  lfd = socket(AF_INET, SOCK_STREAM, 0); // TCP
  if (lfd == -1) {
    std::cerr << "create socket failed" << endl;
    exit(0);
  }
  // 2. 绑定本地端口和IP
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);
  int ret = bind(lfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
  if (ret == -1) {
    std::cerr << "bind failed" << endl;
    exit(0);
  }

  // 3. 设置监听
  ret = listen(lfd, 128);
  if (ret == -1) {
    std::cerr << "listen failed" << endl;
    exit(0);
  }
  // 注册信号的捕捉
  struct sigaction act;
  act.sa_flags = 0;
  act.sa_handler = callback;
  sigemptyset(&act.sa_mask);
  sigaction(SIGCHLD, &act, NULL);

  // 4.等待客户端连接
  socklen_t c_addr_len = sizeof(struct sockaddr_in);
  while (1) {
    cfd = accept(lfd, (struct sockaddr *) (&cliaddr), &c_addr_len);
    if (cfd == -1) {
      if (errno == EINTR) {
        // accept调用被信号中断了, 解除阻塞, 返回了-1
        // 重新调用一次accept
        continue;
      }
      std::cerr << "accept failed" << endl;
      exit(0);
    }

    string client = getIpString(&cliaddr);
    std::cout << client << " connected." << endl;
    if ((child_pid = fork()) == 0) {   /*子进程*/
      close(lfd);    // 子进程不负责监听
      working(cfd, client);  /*处理该客户端的请求*/
      // 退出子进程
      close(cfd);
      exit(0);
    } else if (child_pid > 0) {
      // 父进程不和客户端通信
      close(cfd);
    }
  }
}
//
int working(int cfd, string &client) {
  // 5.与客户端通信
  // 接收数据
  while (1) {
    char buff[BUFFER_SIZE];
    memset(buff, 0, sizeof(buff));
    int len = recv(cfd, buff, sizeof(buff), 0);
    if (len > 0) {
      std::cout << client << " say:" << buff << std::endl;
      send(cfd, buff, len, 0); // 原路发送
    } else if (len == 0) {
      std::cerr << client << " disconnected" << endl;
      break;
    } else {
      std::cerr << client << " read failed" << endl;
      break;
    }
  }
  return 0;
}