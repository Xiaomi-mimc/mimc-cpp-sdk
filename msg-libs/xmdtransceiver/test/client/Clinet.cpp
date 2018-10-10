#include "XMDTransceiver.h"
#include <iostream>
#include "handler.h"

int main(int argc, char *argv[]) {
    XMDTransceiver* transceiver = new XMDTransceiver(1, 44324);
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new streamClientHandler(transceiver));

    transceiver->run();

    char* ip = argv[1];
    int port = atoi(argv[2]);
    int times = atoi(argv[3]);
    int size = atoi(argv[4]);
    int qps = atoi(argv[5]);
    char data[size];
    
    
    //std::string ip = "10.231.49.218";
    
    uint64_t connId = transceiver->createConnection(ip, port, 
                                   NULL, 0, 20, NULL, true);
                                   
    usleep(500000);

    uint16_t streamId = transceiver->createStream(connId, ACK_STREAM, 10);

    
    

    std::cout<<"time="<<times<<",size="<<size<<",qps="<<qps<<",ip="<<ip<<std::endl;

    for (int i = 0; i < times; i++) {
        for (int j = 0; j < size; j++) {
            data[j] = rand() & 0x000000FF;
        }

        uint32_t index = i;
        memcpy(data, &index, 4);
        uint64_t time = current_ms();
        memcpy(data + 4, &time, 8);

        
        transceiver->sendRTData(connId, streamId, data, size);

        usleep(1000000 / qps);
    }

    usleep(5000000);
    std::cout<<"close"<<std::endl;

    transceiver->closeStream(connId, streamId);
    usleep(20000);
    transceiver->closeConnection(connId);


    usleep(2000000);
    

    transceiver->stop();
    delete transceiver;
}


