/*************************************************************************
  > File Name: server.c
  > Author: ljh
 ************************************************************************/

/*
Linux网络编程之基于UDP实现可靠的文件传输示例
这篇文章主要介绍了Linux网络编程之基于UDP实现可靠的文件传输示例,是很实用的技巧,需要的朋友可以参考下
了解网络传输协议的人都知道，采用TCP实现文件传输很简单。相对于TCP，由于UDP是面向无连接、不可靠的传输协议，
所以我们需要考虑丢包和后发先至（包的顺序）的问题，
所以我们想要实现UDP传输文件，则需要解决这两个问题。方法就是给数据包编号，按照包的顺序接收并存储，接收端接收到数据包后发送确认信息给发送端，
发送端接收确认数据以后再继续发送下一个包，如果接收端收到的数据包的编号不是期望的编号，则要求发送端重新发送。
下面展示的是基于linux下C语言实现的一个示例程序，该程序定义一个包的结构体，其中包含数据和包头，包头里包含有包的编号和数据大小，
经过测试后，该程序可以成功传输一个视频文件。
udp传输大文件，解决了udp传输中拆包，组包问题，
1.后包先到
2.数据包确认
3.每包crc32校验(可选，网友说udp解决了顺序组包，应答确认就可以了，协议栈专门有udp校验)
可以说这个udp可靠传输
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <iostream>

#define SERVER_PORT 8888
#define BUFFER_SIZE 500  //发送文件udp缓冲区大小
#define FILE_NAME_MAX_SIZE 512

/* 包头 */
typedef struct {
    long int id;
    int buf_size;
    unsigned int crc32val;  //每一个buffer的crc32值
    int errorflag;
} PackInfo;

/* 接收包 */
struct SendPack {
    PackInfo head;
    char buf[BUFFER_SIZE];
} data;

//----------------------crc32----------------
static unsigned int crc_table[256];
static void init_crc_table(void);
static unsigned int crc32(unsigned int crc, unsigned char *buffer, unsigned int size);
/* 第一次传入的值需要固定,如果发送端使用该值计算crc校验码, 那么接收端也同样需要使用该值进行计算 */
unsigned int crc = 0xffffffff;

/*
 * 初始化crc表,生成32位大小的crc表
 * 也可以直接定义出crc表,直接查表,
 * 但总共有256个,看着眼花,用生成的比较方便.
 */
static void init_crc_table(void) {
    unsigned int c;
    unsigned int i, j;

    for (i = 0; i < 256; i++) {
        c = (unsigned int)i;

        for (j = 0; j < 8; j++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }

        crc_table[i] = c;
    }
}

/* 计算buffer的crc校验码 */
static unsigned int crc32(unsigned int crc, unsigned char *buffer, unsigned int size) {
    unsigned int i;

    for (i = 0; i < size; i++) {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }

    return crc;
}

//主函数入口
using namespace std;
int main() {
    /* 发送id */
    long int send_id = 0;

    /* 接收id */
    int receive_id = 0;

    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    /* 创建socket */
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_fd == -1) {
        cout << "Create Socket Failed:" << endl;
        exit(1);
    }

    /* 绑定套接口 */
    if (-1 == (bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))) {
        cout << "Server Bind Failed:" << endl;
        exit(1);
    }

    // crc32
    init_crc_table();

    /* 数据传输 */
    while (1) {
        /* 定义一个地址，用于捕获客户端地址 */
        struct sockaddr_in client_addr;
        socklen_t client_addr_length = sizeof(client_addr);

        /* 接收数据 */
        char buffer[BUFFER_SIZE];
        bzero(buffer, BUFFER_SIZE);
        if (recvfrom(server_socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_length) ==
            -1) {
            cout << "Receive Data Failed:" << endl;
            exit(1);
        }

        /* 从buffer中拷贝出file_name */
        char file_name[FILE_NAME_MAX_SIZE + 1];
        bzero(file_name, FILE_NAME_MAX_SIZE + 1);
        strncpy(file_name, buffer, strlen(buffer) > FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(buffer));
        cout << file_name << endl;

        /* 打开文件 */
        FILE *fp = fopen(file_name, "r");
        if (NULL == fp) {
            cout << "File Not Found: " << file_name << endl;
        } else {
            int len = 0;
            /* 每读取一段数据，便将其发给客户端 */
            while (1) {
                PackInfo pack_info;

                bzero((char *)&data, sizeof(data));  // ljh socket发送缓冲区清零

                cout << "receive_id: " << receive_id << endl;
                cout << "send_id: " << send_id << endl;

                if (receive_id == send_id) {
                    ++send_id;

                    if ((len = fread(data.buf, sizeof(char), BUFFER_SIZE, fp)) > 0) {
                        data.head.id = send_id;   /* 发送id放进包头,用于标记顺序 */
                        data.head.buf_size = len; /* 记录数据长度 */
                        data.head.crc32val = crc32(crc, reinterpret_cast<unsigned char *>(data.buf), sizeof(data));
                        cout << "len =: " << len << endl;
                        cout << "data.head.crc32val=: " << data.head.crc32val << endl;
                    // cout <<"data.buf=%s\n",data.buf);

                    resend:
                        if (sendto(server_socket_fd, (char *)&data, sizeof(data), 0, (struct sockaddr *)&client_addr,
                                   client_addr_length) < 0) {
                            cout << "Send File Failed:" << endl;
                            break;
                        }

                        /* 接收确认消息 */
                        recvfrom(server_socket_fd, (char *)&pack_info, sizeof(pack_info), 0,
                                 (struct sockaddr *)&client_addr, &client_addr_length);
                        receive_id = pack_info.id;
                        //如果确认包提示数据错误
                        if (pack_info.errorflag == 1) {
                            pack_info.errorflag = 0;
                            goto resend;
                        }

                        // usleep(50000);

                    } else {
                        break;
                    }
                } else {
                    /* 如果接收的id和发送的id不相同,重新发送 */
                    if (sendto(server_socket_fd, (char *)&data, sizeof(data), 0, (struct sockaddr *)&client_addr,
                               client_addr_length) < 0) {
                        cout << "Send File Failed:" << endl;
                        break;
                    }

                    cout << "repeat send" << endl;

                    /* 接收确认消息 */
                    recvfrom(server_socket_fd, (char *)&pack_info, sizeof(pack_info), 0,
                             (struct sockaddr *)&client_addr, &client_addr_length);
                    receive_id = pack_info.id;

                    // usleep(50000);
                }
            }

            //发送结束包 0字节目的告诉客户端发送完毕
            if (sendto(server_socket_fd, (char *)&data, 0, 0, (struct sockaddr *)&client_addr, client_addr_length) <
                0) {
                cout << "Send 0 char  Failed:" << endl;
                break;
            }
            cout << "sever send file end 0 char" << endl;
            ;

            /* 关闭文件 */
            fclose(fp);
            cout << "File:%s Transfer Successful:" << file_name << endl;

            //清零id，准备发送下一个文件
            /* 发送id */
            send_id = 0;
            /* 接收id */
            receive_id = 0;
        }
    }

    close(server_socket_fd);
    return 0;
}
