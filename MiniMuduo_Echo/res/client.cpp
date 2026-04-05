#include "common.h"

const char* SERVER_IP = "192.168.230.128";

int main()
{
    // 1. 创建TCP客户端socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        perror("socket create failed");
        return -1;
    }

    // 2. 配置服务端地址结构体
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // IP地址转换
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton error");
        close(client_fd);
        return -1;
    }

    // 3. 连接服务端
    if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect server failed");
        close(client_fd);
        return -1;
    }
    std::cout << "连接服务端成功！输入内容回车发送(exit退出)\n";

    // 4. 循环交互
    char send_buf[BUF_READ_SIZE] = {0};
    char recv_buf[BUF_READ_SIZE] = {0};
    while (true)
    {
        // 清空缓冲区
        memset(send_buf, 0, BUF_READ_SIZE);
        memset(recv_buf, 0, BUF_READ_SIZE);

        // 读取控制台输入
        std::cout << "请输入消息：";
        std::cin.getline(send_buf, BUF_READ_SIZE - 2);

        // 输入exit主动退出客户端
        if (strcmp(send_buf, "exit") == 0)
        {
            break;
        }

        // 关键：拼接换行符 \n 适配服务端拆包协议
        strcat(send_buf, "\n");

        // 发送数据到服务端
        ssize_t send_len = send(client_fd, send_buf, strlen(send_buf), 0);
        if (send_len <= 0)
        {
            perror("send data failed");
            break;
        }

        // 接收服务端回声数据
        ssize_t recv_len = recv(client_fd, recv_buf, BUF_READ_SIZE, 0);
        if (recv_len <= 0)
        {
            std::cout << "服务端断开连接\n";
            break;
        }

        // 打印回声结果
        std::cout << "服务端回声：" << recv_buf;
    }

    // 5. 关闭套接字释放资源
    close(client_fd);
    std::cout << "客户端已退出\n";
    return 0;
}
