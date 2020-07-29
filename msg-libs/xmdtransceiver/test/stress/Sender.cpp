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
uint64_t send_count = 0, send_success_count = 0, send_fail_count = 0, recv_count = 0;
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
    }

    virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        send_success_count++;
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
    if (argc != 8) {
        std::cout << "./Sender localPort remoteIP remotePort conn_count single_data_len qps run_time_sec" << std::endl;
        return 0;
    }

    uint16_t localPort = atoi(argv[1]);
    string remoteIP(argv[2]);
    uint16_t remotePort = atoi(argv[3]);
    uint64_t conn_count = atoi(argv[4]);
    uint64_t single_data_len = atoi(argv[5]);
    uint64_t qps = atoi(argv[6]);
    uint64_t run_time_sec = atoi(argv[7]);

    XMDTransceiver* transceiver = new XMDTransceiver(20, localPort);
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_INFO);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());

    transceiver->run();

    std::string message = "test_send_createconn";
    for(size_t i = 0; i < conn_count; i++) {
        transceiver->createConnection((char*)remoteIP.c_str(), remotePort, (char*)message.c_str(), message.length(), 100, NULL);
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

    char *message3 = (char*)malloc(single_data_len);
    memset(message3, '1', single_data_len);

    const uint64_t start_time = current_ms();

    const uint64_t start_time_us = current_us();
    for(size_t i = 1; i <= qps * run_time_sec; i++) {        
        uint32_t *pkg_id = (uint32_t*)message3;
        uint64_t *pkg_usec = (uint64_t*)(message3 + sizeof(uint32_t));
        uint64_t send_time = current_ms();
        *pkg_id = send_count;
        *pkg_usec = send_time;
        
        pthread_mutex_lock(&stressMutex);
        size_t n = i % (conn_success.size());
        transceiver->sendRTData(conn_success[n], stream[n], message3, single_data_len, false, P0, 0);
        pthread_mutex_unlock(&stressMutex);

        send_count++;
        if (send_count % 1000 == 0) {
            std::cout << "send data count: " << send_count << std::endl;
        }

        uint64_t actual_all_cost_us = current_us() - start_time_us;
        uint64_t expected_all_cost_us = (1000.0 * 1000.0 * 1.0 / (double)qps) * i;
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
    
    transceiver->stop();
    transceiver->join();
    delete transceiver;

    sleep(2);

    std::cout << "\n\n---------------------performance result--------------------" << std::endl;
    std::cout << "create conn count: " << conn_count << ". success count: " << conn_success.size() << ". failed conn count: " << create_conn_fail << std::endl;
    std::cout << "send " << qps * run_time_sec << " datas in " << fixed << setprecision(2) << (float)(finish_time - start_time) / 1000 << " secs, dataSize " 
              << single_data_len << " Bytes, QPS " << fixed << setprecision(2) << (float)(qps * run_time_sec * 1000) / (finish_time - start_time) << std::endl;
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
