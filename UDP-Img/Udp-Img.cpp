//
// Created by parallels on 7/18/19.
//

#ifndef SENSORPOSECALIBRATION_UDPSERVER_H
#define SENSORPOSECALIBRATION_UDPSERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <memory>
#include <sstream>

enum SaveCommand { SAVE_NONE = 0, SAVE_NORMAL_IMAGE = 1, SAVE_SPECIAL_IMAGE = 13 };

typedef struct tagUDPHeader {
    long int serial_number;    //
    int type;                  // image, text or command
    SaveCommand save_command;  // save command: 0=ignore, 1=save normal, 2=save special
    unsigned long image_size;  // image_size
    unsigned int crc32val;     // check bit
    int errorflag;             // check fail flage: 1-true-error, 0-false-no error
} UDPHeader;

// original header
typedef struct tagTransmitData {
    UDPHeader udpHeader;   // one UDPHeader to contain the transmit data information
    char data_area[1400];  // including the header and image
} TransmitData;

/**
 * @brief The UDPServer class
 */
class UDPServer {
   public:
    UDPServer(int port_number);
    ~UDPServer();

    int InitSocket();
    int Bind();
    int RecvFrom(char* msg_recv_buffer, int buffer_size);
    int SendTo(char* msg_send_buffer, int length);
    int CloseSocket();
    void Init_crc_table(void);                                                       // initial check table
    unsigned int Crc32(unsigned int crc, unsigned char* buffer, unsigned int size);  // get the check value
    int RecvOneImg();
    int IsRecvSstreamEmpty();
    int ShowAndSendSaveCommand();
    // broadcast
    int BroadCast();

   private:
    int port_number_;                 // port number
    int sock_server_fd_;              // the socket number
    struct sockaddr_in server_addr_;  // server address struct
    struct sockaddr_in client_addr_;  // client address struct
    std::stringstream recvSstream_;   // one complete image that consists of multiple data_.data_area
    TransmitData data_;               // the data passed from client once, that contain piece image information
    int recvBufferSize_;              // the buffer size of one data_.data_area
    unsigned int crc_table_[256];     // check value table
    unsigned int crc_;                // check initial value
};

#endif  // SENSORPOSECALIBRATION_UDPSERVER_H

//
// Created by parallels on 7/18/19.
//
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include "highgui.h"
#include "opencv2/opencv.hpp"

/**
 * @brief UDPServer::UDPServer
 * @param port_number
 */
UDPServer::UDPServer(int port_number) {
    port_number_ = port_number;
    recvBufferSize_ = 500;
    crc_ = 0xffffffff;

    InitSocket();

    // BroadCast();
}

/**
 * @brief UDPServer::~UDPServer
 */
UDPServer::~UDPServer() { CloseSocket(); }

/**
 * @brief UDPServer::InitSocket
 * @return
 */
int UDPServer::InitSocket() {
    // complete the key words
    memset(&server_addr_, 0, sizeof(sockaddr_in));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(port_number_);

    // create the socket
    if ((sock_server_fd_ = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return 0;
    }

    // set port re-usable
    int on = 1;
    setsockopt(sock_server_fd_, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

    //    int nRecvBuf = 32 * 1024;  //设置为32K
    //    setsockopt(sock_server_fd_, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

    return 1;
}

/**
 * @brief UDPServer::Bind
 * @return
 */
int UDPServer::Bind() {
    // server_addr_.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock_server_fd_, (struct sockaddr*)&server_addr_, sizeof(struct sockaddr)) < 0) {
        perror("bind error");
        return 0;
    }
    return 1;
}

/**
 * @brief UDPServer::RecvFrom
 * @param msg_recv_buffer
 * @param buffer_size
 * @return
 */
int UDPServer::RecvFrom(char* msg_recv_buffer, int buffer_size) {
    int len = 0;
    int sin_size = sizeof(sockaddr_in);
    if ((len = recvfrom(sock_server_fd_, msg_recv_buffer, buffer_size, 0, (struct sockaddr*)&client_addr_,
                        (socklen_t*)&sin_size)) < 0) {
        perror("recvfrom error");
        return 0;
    }
    // printf("received packet from %s:/n",inet_ntoa(remote_addr.sin_addr));
    if (len < buffer_size) msg_recv_buffer[len] = '/0';

    return 1;
}

/**
 * @brief UDPServer::SendTo
 * @param msg_send_buffer
 * @param length
 * @return
 */
int UDPServer::SendTo(char* msg_send_buffer, int length) {
    int len = 0;
    if ((len = sendto(sock_server_fd_, msg_send_buffer, length, 0, (struct sockaddr*)&client_addr_,
                      sizeof(struct sockaddr))) < 0) {
        perror("sendto error");
        return 0;
    }

    return 1;
}

/**
 * @brief UDPServer::CloseSocket
 * @return
 */
int UDPServer::CloseSocket() {
    close(sock_server_fd_);

    return 1;
}

/**
 * @brief UDPServer::Init_crc_table
 */
void UDPServer::Init_crc_table(void) {
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
        crc_table_[i] = c;
    }
}

/**
 * @brief UDPServer::Crc32
 * @param crc
 * @param buffer
 * @param size
 * @return
 */
unsigned int UDPServer::Crc32(unsigned int crc, unsigned char* buffer, unsigned int size) {
    unsigned int i;

    for (i = 0; i < size; i++) {
        crc = crc_table_[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
        // std::cout << "i = " << i << ", crc = " << crc << std::endl;
    }

    return crc;
}

/**
 * @brief UDPServer::RecvOneImg
 * @return
 */
int UDPServer::RecvOneImg() {
    // preparation
    long int id = 1;
    unsigned int crc32tmp;
    Init_crc_table();
    int len = 0;
    recvSstream_.str("");

    // begin to receive
    while (1) {
        UDPHeader pack_info;  // define a package information struct
                              // pack_info

        // receive once
        len = RecvFrom((char*)&data_, sizeof(data_));
        if ((len == 1) && (data_.udpHeader.image_size != 0)) {
            std::cout << "len = " << len << std::endl;
            // calculate the check bit
            // crc32tmp = Crc32(crc_, reinterpret_cast<unsigned char*>(data_.data_area), sizeof(data_.data_area));
            std::cout << "-------------------------" << std::endl;
            std::cout << "data_.udpHeader.serial_number= " << data_.udpHeader.serial_number << std::endl;
            std::cout << "id= " << id << std::endl;

            if (data_.udpHeader.serial_number == id) {
                std::cout << "crc32tmp= " << crc32tmp << std::endl;
                std::cout << "data.udpHeader.crc32val= " << data_.udpHeader.crc32val << std::endl;

                // check the check value
                if (data_.udpHeader.crc32val == crc32tmp) {
                    std::cout << "rec data success" << std::endl;

                    pack_info.serial_number = data_.udpHeader.serial_number;
                    pack_info.image_size = data_.udpHeader.image_size;  // image size
                    ++id;                                               // receive check complete, result = right
#ifdef NO_VERIFICATION
                    // send receive verification information
                    if (SendTo((char*)&pack_info, sizeof(pack_info)) == 0) {
                        std::cout << "Send confirm information failed!" << std::endl;
                    }
                    std::cout << "Receive verification information has been sent to client" << std::endl;
#endif
                    // press the received image segment into the string stream in order
                    recvSstream_.write(data_.data_area, sizeof(data_.data_area));
                    std::cout << "recSstram size = " << recvSstream_.tellp() << std::endl;
                    //                        /* 写入文件 */
                    //                        if (fwrite(data_.data_area, sizeof(char), data_.udpHeader.image_size, fp)
                    //                        <
                    //                        data.udpHead.image_size) {
                    //                            std::cout << "File Write Failed: " << saveName << std::endl;
                    //                            break;
                    //                        }
                }
#ifdef NO_VERIFICATION
                else {  // wrong package，send the re-send information to client to ask re-send
                    // prepare the re-send package infromation
                    pack_info.serial_number = data_.udpHeader.serial_number;
                    pack_info.image_size = data_.udpHeader.image_size;
                    pack_info.errorflag = 1;

                    std::cout << "rec data error,need to send again" << std::endl;
                    // send the order to ask re-send
                    if (SendTo((char*)&pack_info, sizeof(pack_info)) == 0) {
                        std::cout << "Send confirm information failed!" << std::endl;
                    }
                }
#endif

            }
#ifdef NO_VERIFICATION
            else if (data_.udpHeader.serial_number < id) {  // re-sended package
                pack_info.serial_number = data_.udpHeader.serial_number;
                pack_info.image_size = data_.udpHeader.image_size;
                pack_info.errorflag = 0;  // reset the error flag

                std::cout << "data_.udpHeader.serial_number < id" << std::endl;
                // send the verification information to tell the re-send package has been received
                if (SendTo((char*)&pack_info, sizeof(pack_info)) == 0) {
                    std::cout << "Send confirm information failed!" << std::endl;
                }
            }
#endif
        } else if ((len == 1) && (data_.udpHeader.image_size == 0)) {  // receive all
            if (recvSstream_.tellp() != 0) {
                std::cout << "Receive File From Client Successful" << std::endl;
                // ShowAndSendSaveCommand();
                break;
            }
        }
    }

    return 1;
}

int UDPServer::BroadCast() {
    std::cout << "Server Begin Broadcast ..." << std::endl;
    setvbuf(stdout, NULL, _IONBF, 0);
    fflush(stdout);
    // create the object socket sock
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        std::cout << "sock error" << std::endl;
        return -1;
    }
    // set the type of socket
    const int opt = 1;
    int nb = 0;
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));
    if (nb == -1) {
        std::cout << "set socket error...\n" << std::endl;
        return -1;
    }
    // newly create a sockaddr_in object
    struct sockaddr_in addrto;
    bzero(&addrto, sizeof(struct sockaddr_in));
    addrto.sin_family = AF_INET;
    addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);  // Broadcast address
    addrto.sin_port = htons(6000);                     // port set as 6000
    int nlen = sizeof(addrto);
    socklen_t len = sizeof(addrto);
    // Begin broadcast
    char msg[] = {"server broadcast"};
    int ret = sendto(sock, msg, strlen(msg), 0, (sockaddr*)&addrto, nlen);  //向广播地址发布消息
    if (ret == 0) {
        std::cout << "broadcast error...\n" << std::endl;
        return 0;
    }
    // receive stop broadcast information from client
    char recvMsg[5] = {0};
    ret = recvfrom(sock, recvMsg, 5, 0, (struct sockaddr*)&addrto, &len);
    // close socket
    if ((ret == 0) && (strcmp(recvMsg, "1") == 0)) {
        close(sock);
        return 1;
    }
    return 0;
}

/**
 * @brief UDPServer::ShowAndSave
 * @return
 */
int UDPServer::ShowAndSendSaveCommand() {
    std::cout << "show the image" << std::endl;
    std::string showStr = recvSstream_.str();
    std::cout << "total string stream size = " << recvSstream_.tellp() << std::endl;
    std::vector<char> vec_data(showStr.c_str(), showStr.c_str() + showStr.size());
    cv::Mat img = cv::imdecode(vec_data, CV_LOAD_IMAGE_UNCHANGED);
    std::cout << "Server: img.size = " << img.size << std::endl;
    std::cout << "Server: rows*columns = " << img.rows << " * " << img.cols << std::endl;

    // show
    namedWindow("Show", cv::WINDOW_NORMAL);
    cv::imshow("Show", img);

    // send the save command
    // prepare the package
    UDPHeader packInfo;
    packInfo.save_command = SaveCommand::SAVE_NONE;
    std::cout << "packInfo.saveCommand = " << packInfo.save_command << std::endl;
    if (cv::waitKey(100) == 27) {  // ESC
        return 0;
    } else if (cv::waitKey(300) == 13) {  // Enter
        packInfo.save_command = SaveCommand::SAVE_NORMAL_IMAGE;
        std::cout << "Save image" << std::endl;
    } else if (cv::waitKey(300) == 32) {  // SpaceBar
        packInfo.save_command = SaveCommand::SAVE_SPECIAL_IMAGE;
    } else {
        packInfo.save_command = SaveCommand::SAVE_NONE;
    }
    // send save image command
    std::cout << "save(1)?: " << packInfo.save_command << std::endl;
    if (SendTo((char*)&packInfo, sizeof(packInfo)) == 0) {
        std::cout << "Send save command failed!" << std::endl;
    } else {
        std::cout << "Send save command!" << std::endl;
#ifdef NO_VERIFICATION
        // receive saved information from client
        RecvFrom((char*)&packInfo, sizeof(packInfo));
        if (packInfo.save_command == 2) {
            std::cout << "the image has been saved" << std::endl;
        }
#endif
    }

    return 1;
}

/**
 * @brief UDPServer::IsRecvSstreamEmpty
 * @return
 */
int UDPServer::IsRecvSstreamEmpty() {
    if (recvSstream_.tellp() == 0) {
        return 0;
    }
    return 1;
}

// main
#include <string>
#include <vector>
#include "errno.h"
#include "highgui.h"
#include "opencv2/opencv.hpp"
int main(int argc, char* argv[]) {
    // create connection
    UDPServer udpServer(7777);
    // broad cast the ip address
    udpServer.BroadCast();
    // Bind the socket
    udpServer.Bind();
    // receive images
    while (1) {
        // receive one image
        std::cout << "Server turned on" << std::endl;
        udpServer.RecvOneImg();
        if (udpServer.IsRecvSstreamEmpty() != 0) {
            // show the image and send save command
            int flag = udpServer.ShowAndSendSaveCommand();
            if (flag == 0) {
                std::cout << "exit show" << std::endl;
                break;
            }
            std::cout << "exit show" << std::endl;
        }
    }
    udpServer.CloseSocket();
    return 1;
}
