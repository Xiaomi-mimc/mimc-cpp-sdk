#include "XMDTransceiver.h"
#include <iostream>
#include "handler.h"
#include <sys/prctl.h>


int main(int argc, char *argv[]) {
    char tname[16];
    memset(tname, 0, 16);
    strcpy(tname, "xmd client");
    prctl(PR_SET_NAME, tname);

    char* ip = argv[1];
    int port = atoi(argv[2]);
    int times = atoi(argv[3]);
    int size = atoi(argv[4]);
    int qps = atoi(argv[5]);
    int packetLossRate = atoi(argv[6]);
    char data[size];
    std::cout<<"time="<<times<<",size="<<size<<",qps="<<qps<<",ip="<<ip<<",rate="<<packetLossRate<<std::endl;
    
    for (int i = 0; i < 1; i++) {
    XMDTransceiver* transceiver = new XMDTransceiver(1, 44324);
    transceiver->start();
    //transceiver->SetPingTimeIntervalSecond(10);
    DataGramHandler* datagram_handler = new DataGramHandler();
    newConn* conn_handler = new newConn();
    streamClientHandler* stream_handler = new streamClientHandler(transceiver);
    SocketErrHander* socket_err_handler = new SocketErrHander(transceiver);
    transceiver->registerRecvDatagramHandler(datagram_handler);
    transceiver->registerConnHandler(conn_handler);
    transceiver->registerStreamHandler(stream_handler);
    transceiver->registerSocketErrHandler(socket_err_handler);
    transceiver->run();

    std::string ctx_str = "conn ctx 123343333333333333333";
    char* conn_ctx = new char[100];
    memcpy(conn_ctx, ctx_str.c_str(), ctx_str.length());

    std::cout<<"create conn ctx="<<conn_ctx<<std::endl;
    
    uint64_t connId = transceiver->createConnection(ip, port, NULL, 0, 3, conn_ctx);
    usleep(100000);
    uint16_t streamId = transceiver->createStream(connId, BATCH_ACK_STREAM, 400, false);

    transceiver->setTestPacketLoss(packetLossRate);
    transceiver->setXMDLogLevel(XMD_INFO);

    for (int i = 0; i < times; i++) {
        for (int j = 0; j < size; j++) {
            data[j] = rand() & 0x000000FF;
        }

        uint32_t index = i;
        memcpy(data, &index, 4);
        uint64_t time = current_ms();
        memcpy(data + 4, &time, 8);

        char* tmptest = new char[20];
        int dataid = transceiver->sendRTData(connId, streamId, data, size, true, P0, -1, tmptest);
        if (dataid < 0) {
            delete[] tmptest;
        }
        usleep(1000000 / qps);
    }

    
    std::cout<<"close"<<std::endl;


    usleep(15000000);
    transceiver->closeStream(connId, streamId);
    transceiver->closeConnection(connId);
    usleep(1500000);
    transceiver->stop();
    transceiver->join();
    delete transceiver;
    delete datagram_handler;
    delete conn_handler;
    delete stream_handler;
    delete socket_err_handler;
    }
    usleep(200000);
}


