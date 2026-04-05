#include "common.h"

//工具函数：根据协议分隔符解析协议包
std::vector<std::string>splitPacketByline(std::string& recvBuf){
    std::vector<std::string>res;
    size_t pos;
    //循环查找协议分隔符
    while((pos = recvBuf.find(LINE_SEP)) != std::string::npos){
        std::string packet = recvBuf.substr(0, pos); //提取协议包
        res.push_back(packet); //存储协议包
        recvBuf=recvBuf.substr(pos + 1); //更新缓冲区，去掉已处理的协议包
    }
    return res;
}

//简易线程池模块
void* workThreadFunc(void* arg){
    (void)arg; //避免未使用参数的警告
    while(true){
        int cliFd;
        //获取任务
        pthread_mutex_lock(&qunenMutex);
        while(taskQueue.empty()){
            pthread_cond_wait(&queueCond, &qunenMutex); //等待任务到来
        }
        cliFd = taskQueue.front();
        taskQueue.pop();
        pthread_mutex_unlock(&qunenMutex);
        
        //独立私有缓冲区，解决TCP粘包问题
        std::string recvBuf;
        char tempbuf[BUF_READ_SIZE];
        //处理客户端请求
        while(true){
            memset(tempbuf, 0, sizeof(tempbuf));
            //阻塞接收数据
            int recvlen = recv(cliFd, tempbuf, sizeof(tempbuf), 0);
            if(recvlen<=0){
                std::cout<<"线程池断开连接"<<std::endl;
                close(cliFd);
                break;
            }
            recvBuf.append(tempbuf, recvlen); //将接收的数据追加到缓冲区
            //解析协议包
            std::vector<std::string>packets = splitPacketByline(recvBuf);
            for(const auto& msg : packets){
                std::cout<<"线程池处理协议包: "<<msg<<std::endl;
                std::string echoMsg = msg + LINE_SEP; //回显协议包
                send(cliFd, echoMsg.c_str(), echoMsg.size(), 0); //发送
            }
        }
    }
    return nullptr;
}

//初始化线程池
void initThreadPool(){
    pthread_mutex_init(&qunenMutex, nullptr);
    pthread_cond_init(&queueCond, nullptr);
    pthread_t tid;
    for(int i=0; i<THREAD_POOL_SIZE; ++i){  
        pthread_create(&tid, nullptr, workThreadFunc, nullptr); //创建线程
        pthread_detach(tid); //分离线程，自动回收资源
    }
    std::cout<<"线程池初始化完成，线程数: "<<THREAD_POOL_SIZE<<std::endl;
}

//将任务添加到线程池
void addTaskToPool(int cliFd){
    pthread_mutex_lock(&qunenMutex);
    taskQueue.push(cliFd); //将客户端连接描述符加入任务队列
    pthread_cond_signal(&queueCond); //通知线程池有新任务
    pthread_mutex_unlock(&qunenMutex);
}

//epoll主服务模块，负责监听和分发客户端连接
int createListtenFd(){
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd == -1){
        std::cerr<<"创建监听套接字失败: "<<strerror(errno)<<std::endl;
        return -1;
    }
    //设置套接字选项，允许地址重用
    int opt = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //绑定地址和端口
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);  

    if (bind(listenFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1){
        std::cerr<<"绑定失败: "<<strerror(errno)<<std::endl;
        close(listenFd);
        exit(-1);
    }
    //开始监听
    if (listen(listenFd, SOMAXCONN) == -1){
        std::cerr<<"监听失败: "<<strerror(errno)<<std::endl;
        close(listenFd);
        exit(-1);
    }
    std::cout<<"服务器启动，监听端口: "<<PORT<<std::endl;
    return listenFd;
}

int main(){
    int listenFd = createListtenFd();
    if(listenFd < 0){
        return -1;
    }
    initThreadPool(); //初始化线程池

    //创建epoll实例
    int epollFd = epoll_create1(0);
    if(epollFd < 0){
        std::cerr<<"创建epoll实例失败: "<<strerror(errno)<<std::endl;
        close(listenFd);
        return -1;
    }
    //将监听套接字添加到epoll事件列表
    epoll_event ev;
    ev.events = EPOLLIN; //可读事件
    ev.data.fd = listenFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &ev) == -1){
        std::cerr<<"添加监听套接字到epoll失败: "<<strerror(errno)<<std::endl;
        close(listenFd);
        close(epollFd);
        return -1;
    }

    epoll_event events[MAX_EPOLL_EVENTS];
    while(true){
        int nfds = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, -1); //等待事件发生
        if(nfds == -1){
            std::cerr<<"epoll_wait失败: "<<strerror(errno)<<std::endl;
            break;
        }
        for(int i=0; i<nfds; ++i){
            if(events[i].data.fd == listenFd){ //新客户端连接
                sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                int cliFd = accept(listenFd, (sockaddr*)&clientAddr, &clientAddrLen);
                if(cliFd == -1){
                    std::cerr<<"接受连接失败: "<<strerror(errno)<<std::endl;
                    continue;
                }
                std::cout<<"新客户端连接: "<<inet_ntoa(clientAddr.sin_addr)<<":"<<ntohs(clientAddr.sin_port)<<std::endl;
                addTaskToPool(cliFd); //将新连接添加到线程池处理
            }
        }
    }
    close(listenFd);
    close(epollFd);
    return 0;
}
    