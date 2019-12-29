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
#include <iostream>
#include <sstream>
#include <vector>
#include "highgui.h"
#include "opencv2/opencv.hpp"

#define SERVER_PORT 7777
#define BUFFER_SIZE 500  //发送文件udp缓冲区大小
#define FILE_NAME_MAX_SIZE 512
#define IMAGE_SIZE 300 * 1024 * 8

/* 包头 */
typedef struct {
    long int id;
    int buf_size;
    unsigned int crc32val;  //每一个buffer的crc32值
    int errorflag;          // 1=error, 0=no error
    int saveCommand;        // 1=save, 0=ignore
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
    long int id;
    unsigned int crc32tmp;
    int imgSerialNum = 0;

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

    string path = "/home/beyoung/Desktop/mac-ubuntu-share/1_IDSCameraDev/5_SAPro_ImageSave100/";
    string savePath = "/home/beyoung/Desktop/mac-ubuntu-share/1_IDSCameraDev/6_UDPBigFileSave/";

    for (imgSerialNum = 0; imgSerialNum >= 0; ++imgSerialNum) {
        id = 1;
        // crc32
        init_crc_table();

        /* 打开文件，准备写入 */
        string saveName = savePath + to_string(imgSerialNum) + ".jpg";
        FILE *fp = fopen(saveName.c_str(), "w");
        if (NULL == fp) {
            cout << "File Can Not Open To Write: " << saveName << endl;
            exit(1);
        }

        /* 从服务器接收数据，并写入文件 */
        int len = 0;
        stringstream recSstream;

        /* 定义一个地址，用于捕获客户端地址 */
        struct sockaddr_in client_addr;
        socklen_t client_addr_length = sizeof(client_addr);

        while (1) {
            PackInfo pack_info;  //定义确认包变量

            // communication
            if ((len = recvfrom(server_socket_fd, (char *)&data, sizeof(data), 0, (struct sockaddr *)&client_addr,
                                &client_addr_length)) > 0) {
                cout << "len = " << len;
                crc32tmp = crc32(crc, reinterpret_cast<unsigned char *>(data.buf), sizeof(data));

                // crc32tmp=5;
                cout << "-------------------------" << endl;
                cout << "data.head.id= " << data.head.id << endl;
                cout << "id= " << id << endl;

                if (data.head.id == id) {
                    cout << "crc32tmp= " << crc32tmp << endl;
                    cout << "data.head.crc32val= " << data.head.crc32val;
                    // cout <<"data.buf=%s\n",data.buf);

                    //校验数据正确
                    if (data.head.crc32val == crc32tmp) {
                        cout << "rec data success" << endl;

                        pack_info.id = data.head.id;
                        pack_info.buf_size = data.head.buf_size;  //文件中有效字节的个数，作为写入文件fwrite的字节数
                        ++id;                                     //接收正确，准备接收下一包数据

                        /* 发送数据包确认信息 */
                        if (sendto(server_socket_fd, (char *)&pack_info, sizeof(pack_info), 0,
                                   (struct sockaddr *)&client_addr, client_addr_length) < 0) {
                            cout << "Send confirm information failed!" << endl;
                        }
                        // receive buffer and copy it into recBuffer
                        string recString(data.buf);
                        recSstream.write(data.buf, sizeof(data.buf));
                        cout << "recSstram size = " << sizeof(recSstream) << ", " << recSstream.tellp() << endl;
                        //                        /* 写入文件 */
                        //                        if (fwrite(data.buf, sizeof(char), data.head.buf_size, fp) <
                        //                        data.head.buf_size) {
                        //                            cout << "File Write Failed: " << saveName << endl;
                        //                            break;
                        //                        }
                    } else {
                        pack_info.id = data.head.id;  //错误包，让服务器重发一次
                        pack_info.buf_size = data.head.buf_size;
                        pack_info.errorflag = 1;

                        cout << "rec data error,need to send again" << endl;
                        /* 重发数据包确认信息 */
                        if (sendto(server_socket_fd, (char *)&pack_info, sizeof(pack_info), 0,
                                   (struct sockaddr *)&client_addr, client_addr_length) < 0) {
                            cout << "Send confirm information failed!" << endl;
                        }
                    }

                } else if (data.head.id < id) /* 如果是重发的包 */
                {
                    pack_info.id = data.head.id;
                    pack_info.buf_size = data.head.buf_size;

                    pack_info.errorflag = 0;  //错误包标志清零

                    cout << "data.head.id < id" << endl;
                    /* 重发数据包确认信息 */
                    if (sendto(server_socket_fd, (char *)&pack_info, sizeof(pack_info), 0,
                               (struct sockaddr *)&client_addr, client_addr_length) < 0) {
                        cout << "Send confirm information failed!" << endl;
                    }
                }
            } else  //接收完毕退出
            {
                break;
            }
        }

        cout << "Receive File From Server IP Successful: " << saveName << endl;
        // convert sstream to Mat to show by opencv
        string showStr = recSstream.str();
        cout << "total string stream size = " << recSstream.tellp() << endl;
        if (recSstream.tellp() == 0) {
            continue;
        }
        vector<char> vec_data(showStr.c_str(), showStr.c_str() + showStr.size());
        for (auto iter = vec_data.begin(); iter != vec_data.end(); ++iter) {
            std::cout << iter.base() << std::endl;
        }

        cv::Mat img = cv::imdecode(vec_data, CV_LOAD_IMAGE_UNCHANGED);
        std::cout << "img size = " << img.size() << std::endl;

        // from saved image
        // cv::Mat img = cv::imread(saveName, CV_LOAD_IMAGE_COLOR);

        // show
        if (img.empty()) {
            cout << "image: " << saveName << " is empty" << endl;
            continue;
        }
        namedWindow("Show", cv::WINDOW_NORMAL);
        cv::imshow("Show", img);
        PackInfo packInfo;
        packInfo.saveCommand = 0;
        cout << "packInfo.saveCommand = " << packInfo.saveCommand << endl;
        if (cv::waitKey(500) == 27) {
            break;
        } else if (cv::waitKey(500) == 13) {
            packInfo.saveCommand = 1;
            cout << "Save image : " << saveName << endl;
        } else {
            packInfo.saveCommand = 0;
        }
        // send save image command
        cout << "save(1)?: " << packInfo.saveCommand << endl;
        if (sendto(server_socket_fd, (char *)&packInfo, sizeof(packInfo), 0, (struct sockaddr *)&client_addr,
                   client_addr_length) < 0) {
            cout << "Send confirm information failed!" << endl;
        }
        //        cv::destroyWindow("Show");
        // fclose(fp);
    }
    close(server_socket_fd);
    return 0;
}
