#include "XMDTransceiver.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>
#include <pthread.h>

using namespace std;

std::vector<uint64_t> conn_success;
pthread_mutex_t stressMutex = PTHREAD_MUTEX_INITIALIZER;

uint64_t create_conn_fail = 0;
uint64_t send_count = 0, send_big_data_count = 0, send_success_count = 0, send_fail_count = 0, recv_count = 0;
uint64_t recv_0_50_ms = 0, recv_51_100_ms = 0, recv_101_200_ms = 0, recv_201_300_ms = 0, recv_301_400_ms = 0, recv_401_500_ms = 0;
uint64_t recv_501_600_ms = 0, recv_601_800_ms = 0, recv_801_1000_ms = 0, recv_1001_1500_ms = 0, recv_1501_2000_ms = 0;
uint64_t recv_2001_3000_ms = 0, recv_3001_4000_ms = 0, recv_4001_5000_ms = 0, recv_5001_more_ms = 0;

class DataGramHandler : public DatagramRecvHandler {
public:
    virtual void handle(char* ip, int port, char* data, uint32_t len) {
        //std::cout<<"recv"<<std::endl;
        std::string tmpStr(data, len);
        std::cout<<current_ms()<<":receive datagram.data=" << tmpStr << ",len=" << len << std::endl;
    }

};

class newConn : public ConnectionHandler {
public:
    virtual void NewConnection(uint64_t connId, char* data, int len) { 
    }

    virtual void ConnCreateSucc(uint64_t connId, void* ctx) {
        pthread_mutex_lock(&stressMutex);
        conn_success.push_back(connId);
        pthread_mutex_unlock(&stressMutex);
    }
    virtual void ConnCreateFail(uint64_t connId, void* ctx) {
        std::cout<<"create conn fail, conn id=" << connId<<std::endl;
        create_conn_fail++;
    }
    virtual void CloseConnection(uint64_t connId, ConnCloseType type) { 
    }
    virtual void ConnIpChange(uint64_t connId, std::string ip, int port) {
    }
};

class stHandler : public StreamHandler {
public:
    virtual void NewStream(uint64_t connId, uint16_t streamId) {
    }
    virtual void CloseStream(uint64_t connId, uint16_t streamId) { 
    }
    virtual void RecvStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) {
        pthread_mutex_lock(&stressMutex);
        uint64_t recv_time = current_ms();
        recv_count++;
        if (recv_count % 1000 == 0) {
            std::cout << "recv data count: " << recv_count << std::endl;
        }

        uint32_t pkg_id = *((uint32_t*)data);
        uint64_t pkg_usec = *((uint64_t*)(data + sizeof(uint32_t)));
        uint64_t cost = recv_time - pkg_usec;

        if (cost < 51) {
            recv_0_50_ms++;
        } else if (cost < 101) {
            recv_51_100_ms++;
        } else if (cost < 201) {
            recv_101_200_ms++;
        } else if (cost < 301) {
            recv_201_300_ms++;
        } else if (cost < 401) {
            recv_301_400_ms++;
        } else if (cost < 501) {
            recv_401_500_ms++;
        } else if (cost < 601) {
            recv_501_600_ms++;
        } else if (cost < 801) {
            recv_601_800_ms++;
        } else if (cost < 1001) {
            recv_801_1000_ms++;
        } else if (cost < 1501) {
            recv_1001_1500_ms++;
        } else if (cost < 2001) {
            recv_1501_2000_ms++;
        } else if (cost < 3001) {
            recv_2001_3000_ms++;
        } else if (cost < 4001) {
            recv_3001_4000_ms++;
        } else if (cost < 5001) {
            recv_4001_5000_ms++;
        } else {
            recv_5001_more_ms++;
        }
        pthread_mutex_unlock(&stressMutex);
    }

    virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        pthread_mutex_lock(&stressMutex);
        send_success_count++;
        pthread_mutex_unlock(&stressMutex);
    }
    
    virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        std::cout<<"send stream data fail, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
        send_fail_count++;
    }

    virtual void sendFECStreamDataComplete(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) { 
    }
};

class XMDLOG : public ExternalLog {
public:
    virtual void info(const char *msg) {
        std::cout<<"xmdlog info:"<<msg<<std::endl;
    }

    virtual void debug(const char *msg) {
        std::cout<<"xmdlog debug:"<<msg<<std::endl;
    }

    virtual void warn(const char *msg) {
        std::cout<<"xmdlog warn:"<<msg<<std::endl;
    }

    virtual void error(const char *msg) {
        std::cout<<"xmdlog error:"<<msg<<std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cout << "./Sender localPort remoteIP remotePort conn_count run_time_sec packetLoss" << std::endl;
        return 0;
    }

    uint16_t localPort = atoi(argv[1]);
    string remoteIP(argv[2]);
    uint16_t remotePort = atoi(argv[3]);
    uint64_t conn_count = atoi(argv[4]);
    uint64_t run_time_sec = atoi(argv[5]);
    int packetloss = atoi(argv[6]);

    XMDTransceiver* transceiver = new XMDTransceiver(1, localPort);
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_INFO);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->SetAckPacketResendIntervalMicroSecond(1000 * 1000);
    transceiver->setTestPacketLoss(packetloss);

    transceiver->run();

    std::string message = "test_send_createconn";
    for(size_t i = 0; i < conn_count; i++) {
        transceiver->createConnection((char*)remoteIP.c_str(), remotePort, (char*)message.c_str(), message.length(), 3, NULL);
        usleep(10 * 1000);
    }

    sleep(3);
    std::cout << "success conn count: " << conn_success.size() << ". failed conn count: " << create_conn_fail << std::endl;
    if (conn_success.size() == 0) {
        std::cout << "success conn count is 0" << std::endl;
    }

    std::vector<uint16_t> stream;
    std::string message2 = "test_send_RTDATA";

    pthread_mutex_lock(&stressMutex);
    for(size_t i = 0; i < conn_success.size(); i++) {
        uint16_t streamId = transceiver->createStream(conn_success[i], ACK_STREAM, 1000, false);
        stream.push_back(streamId);
        usleep(10 * 1000);
    }
    pthread_mutex_unlock(&stressMutex);

    uint64_t single_data_len_3 = 1024 * 4;
    uint64_t single_data_len_4 = 640;
    uint64_t single_data_len_5 = 1024 * 200;

    char *message3 = (char*)malloc(single_data_len_3);
    memset(message3, '1', single_data_len_3);

    char *message4 = (char*)malloc(single_data_len_4);
    memset(message4, '1', single_data_len_4);

    char *message5 = (char*)malloc(single_data_len_5);
    memset(message5, '1', single_data_len_5);

    const uint64_t start_time = current_ms();

    const uint64_t start_time_us = xmd_current_us();
    std::cout << "send start at " << start_time_us << std::endl;
    for(size_t i = 1; i <= 20 * run_time_sec; i++) {
        for(size_t n = 0; n < conn_success.size(); n++) {
            uint32_t *pkg_id_3 = (uint32_t*)message3;
            uint64_t *pkg_usec_3 = (uint64_t*)(message3 + sizeof(uint32_t));
            uint64_t send_time = current_ms();
            *pkg_id_3 = send_count;
            *pkg_usec_3 = send_time;
            
            pthread_mutex_lock(&stressMutex);
            transceiver->sendRTData(conn_success[n], stream[n], message3, single_data_len_3, false, P0, 10);
            pthread_mutex_unlock(&stressMutex);

            send_count++;

            uint32_t *pkg_id_4 = (uint32_t*)message4;
            uint64_t *pkg_usec_4 = (uint64_t*)(message4 + sizeof(uint32_t));
            send_time = current_ms();
            *pkg_id_4 = send_count;
            *pkg_usec_4 = send_time;
            
            pthread_mutex_lock(&stressMutex);
            transceiver->sendRTData(conn_success[n], stream[n], message4, single_data_len_4, false, P0, 10);
            pthread_mutex_unlock(&stressMutex);

            send_count++;

            if (xmd_current_us() - start_time_us > (send_big_data_count + 1) * 1000 * 1000 * 3 / conn_success.size()) {
                std::cout << "time " << xmd_current_us() << ", send big data count " << send_big_data_count << std::endl;
                uint32_t *pkg_id_5 = (uint32_t*)message5;
                uint64_t *pkg_usec_5 = (uint64_t*)(message5 + sizeof(uint32_t));
                send_time = current_ms();
                *pkg_id_5 = send_count;
                *pkg_usec_5 = send_time;

                pthread_mutex_lock(&stressMutex);
                transceiver->sendRTData(conn_success[send_big_data_count % conn_success.size()], stream[send_big_data_count % conn_success.size()], message5, single_data_len_5, false, P0, 0);
                pthread_mutex_unlock(&stressMutex);

                send_big_data_count++;
            }       
        }
    
        uint64_t actual_all_cost_us = xmd_current_us() - start_time_us;
        uint64_t expected_all_cost_us = (1000.0 * 1000.0 * 1.0 / 20) * i;
        if (actual_all_cost_us < expected_all_cost_us) {
            usleep(expected_all_cost_us - actual_all_cost_us);
        }
    }

    uint64_t finish_time = current_ms();
    std::cout << "send data finish at: " << finish_time << std::endl;

    uint32_t recv_count_0 = recv_count;
    std::cout << "recv data: " << recv_count << std::endl;
    sleep(5);
    while (recv_count != recv_count_0) {
        std::cout << "recv data: " << recv_count << std::endl;
        sleep(1);
        recv_count_0 = recv_count;
    }

    for(size_t i = 0; i < conn_success.size(); i++) {
        transceiver->closeConnection(conn_success[i]);
        usleep(10 * 1000);
    }
    
    transceiver->stop();
    transceiver->join();
    delete transceiver;

    sleep(2);

    std::cout << "\n\n---------------------performance result--------------------" << std::endl;
    std::cout << "create conn count: " << conn_count << ". success count: " << conn_success.size() << ". failed conn count: " << create_conn_fail << std::endl;
    std::cout << "send " << 20 * run_time_sec << " " << single_data_len_4 << " Byte datas in " << run_time_sec << "sec, qps 20" << std::endl;
    std::cout << "send " << 20 * run_time_sec << " " << single_data_len_3 << " Byte datas in " << run_time_sec << "sec, qps 20" << std::endl;
    std::cout << "send " << send_big_data_count << " " << single_data_len_5 << " Byte datas in " << run_time_sec << "sec, qps 0.33" << std::endl;
    std::cout << "send count: " << send_count + send_big_data_count << std::endl;          
    std::cout << "send success count: " << send_success_count << std::endl;
    std::cout << "send fail count: " << send_fail_count << std::endl;
    std::cout << "receive data: " << recv_count << std::endl;
    std::cout << "lost data: " << send_success_count - recv_count << " (" << fixed << setprecision(2) << (float)(send_success_count - recv_count) * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time      < 50   ms: " << recv_0_50_ms << " (" << fixed << setprecision(2) << (float)recv_0_50_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time   51 ~ 100  ms: " << recv_51_100_ms << " (" << fixed << setprecision(2) << (float)recv_51_100_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time  101 ~ 200  ms: " << recv_101_200_ms << " (" << fixed << setprecision(2) << (float)recv_101_200_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time  201 ~ 300  ms: " << recv_201_300_ms << " (" << fixed << setprecision(2) << (float)recv_201_300_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time  301 ~ 400  ms: " << recv_301_400_ms << " (" << fixed << setprecision(2) << (float)recv_301_400_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time  401 ~ 500  ms: " << recv_401_500_ms << " (" << fixed << setprecision(2) << (float)recv_401_500_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time  501 ~ 600  ms: " << recv_501_600_ms << " (" << fixed << setprecision(2) << (float)recv_501_600_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time  601 ~ 800  ms: " << recv_601_800_ms << " (" << fixed << setprecision(2) << (float)recv_601_800_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time  801 ~ 1000 ms: " << recv_801_1000_ms << " (" << fixed << setprecision(2) << (float)recv_801_1000_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time 1001 ~ 1500 ms: " << recv_1001_1500_ms << " (" << fixed << setprecision(2) << (float)recv_1001_1500_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time 1501 ~ 2000 ms: " << recv_1501_2000_ms << " (" << fixed << setprecision(2) << (float)recv_1501_2000_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time 2001 ~ 3000 ms: " << recv_2001_3000_ms << " (" << fixed << setprecision(2) << (float)recv_2001_3000_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time 3001 ~ 4000 ms: " << recv_3001_4000_ms << " (" << fixed << setprecision(2) << (float)recv_3001_4000_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time 4001 ~ 5000 ms: " << recv_4001_5000_ms << " (" << fixed << setprecision(2) << (float)recv_4001_5000_ms * 100 / send_count << "%) " << std::endl;
    std::cout << "receive time       5000+ ms: " << recv_5001_more_ms << " (" << fixed << setprecision(2) << (float)recv_5001_more_ms * 100 / send_count << "%) " << std::endl;
}
