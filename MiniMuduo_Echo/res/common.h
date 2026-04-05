#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <vector>
#include <queue>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define MAX_EPOLL_EVENTS 65535   //最大监听数
#define THREAD_POOL_SIZE 64      //线程池数
#define BUF_READ_SIZE 1024      //缓冲区大小    
#define PORT 8888               //服务端口
#define LINE_SEP '\n'           //协议分隔符
#define LOG_FILE "server.log"   //日志记录

// 【仅声明】函数实现放到cpp文件中
std::vector<std::string>splitPacketByline(std::string& recvBuf);
void* workThreadFunc(void* arg);
void initThreadPool();
void addTaskToPool(int cliFd);
int createListtenFd();

static std::queue<int>taskQueue;    //任务队列
static pthread_mutex_t qunenMutex;  //同步锁+条件变量
static pthread_cond_t queueCond;
#endif
