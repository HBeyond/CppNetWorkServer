#include <iostream>
#include "boost/asio.hpp"
#include "boost/thread.hpp"

using namespace boost::asio;

void echoExample() {
    io_service service;

    //    void handle_connections() {
    //        char buff[1024];
    //        ip::udp::socket sock(service, ip::udp::endpoint(ip::udp::v4(), 8001));
    //        while (true) {
    //            ip::udp::endpoint sender_ep;
    //            int bytes = sock.receive_from(buffer(buff), sender_ep);
    //            std::string msg(buff, bytes);
    //            sock.send_to(buffer(msg), sender_ep);
    //        }
    //    }

    // int main(int argc, char* argv[]) { handle_connections(); }
}

/**
 * @brief Data collection command, received from FA and EM, which used to control the data collection process
 */
enum CommandType {
    COMMAND_NONE = 0,
    COMMAND_CAPTURE_NORMAL_IMAGE = 5,
    COMMAND_CAPTURE_SPECIAL_IMAGE = 10,
    COMMAND_BROADCAST = 11,
    COMMAND_BROADCAST_RESPONSE = 12,
    COMMAND_COUNT
};

const int gMaxUdpPakcetSize = 1024 * 30;  // packet size of udp sender
const int resize_scale = 4;               // image resize size
const int kRecvBufferSize = 16;           // command receiver buffer size

#pragma pack(push, 1)  // memory alignment
/**
  @ @brief tag udp header. information of udp packet
 */
typedef struct tagUDPHeader {
    uint32_t uIndex;           // index
    uint32_t uTotalDataLen;    // total data size
    uint32_t uCurrentDataLen;  // current sent size
    uint16_t uSplitCount;      // split count
    uint16_t uCurrentIndex;    // current packet sub index
    uint32_t uCurrentOffset;   // current packet data offset
    uint16_t uFinish;          // finish flag，0：finish，1：not finish
} UDPHeader;

/**
  @ @brief tag transmit data. struct to contain the information transmit to FA
 */
typedef struct tagTransmitData {
    UDPHeader udpHeader;                // one UDPHeader to contain the transmit data information
    char data_area[gMaxUdpPakcetSize];  // including the header and image
} TransmitData;

/**
  @ @brief sa request header. information of the response between FA and Sa without any data
 */
struct SARequestHeader {
    uint32_t request_seq;
    CommandType request_type;
    // save command: 0=ignore, 1=save normal, 2=save special
};
#pragma pack(pop)  // memory alignment

/**
 * @brief The server class
 */
class UDPServer {
   public:
    /**
     * @brief server
     * @param client_ip
     * @param port_number
     * @param service
     */
    UDPServer(std::string& client_ip, int port_number, io_service& service)
        : client_ip_(client_ip),
          port_number_(port_number),
          sock_(service, ip::udp::endpoint(ip::udp::v4(), port_number)) {}

    /**
     * @brief REcvFrom
     */
    void RecvFrom() {
        char recv_msg[gMaxUdpPakcetSize];
        ip::udp::endpoint sender_ep;
        sock_.receive_from(buffer(recv_msg), sender_ep);
        // TransmitData* transData;
        std::cout << "received message is : " << recv_msg << std::endl;
    }

    /**
     * @brief showImage
     */
    void showImage() {}

    /**
     * @brief CloseSocket
     */
    void CloseSocket() { sock_.close(); }

   private:
    std::string client_ip_;
    int port_number_;
    ip::udp::socket sock_;
};

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[]) {
    io_service service;
    std::string client_ip = "127.0.0.1";
    int client_port = 8000;
    UDPServer udpServer(client_ip, client_port, service);
    udpServer.RecvFrom();
    return 0;
}
