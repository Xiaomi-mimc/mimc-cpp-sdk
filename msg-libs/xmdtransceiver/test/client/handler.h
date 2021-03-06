#include "XMDTransceiver.h"
#include <iostream>


class DataGramHandler : public DatagramRecvHandler {
public:
    virtual void handle(char* ip, int port, char* data, uint32_t len, unsigned char packetType) {
        std::string tmpStr(data, len);
        std::cout<<current_ms()<<":receive datagram.data=" << tmpStr << ",len=" << len << std::endl;
    }

};

class newConn : public ConnectionHandler {
public:
    virtual void NewConnection(uint64_t connId, char* data, int len) { 
        std::string tmpStr(data, len);
        std::cout<<"new connection,connid=" <<connId<<",data="<< tmpStr<<",len="<<len<<std::endl;
    }

    virtual void ConnCreateSucc(uint64_t connId, void* ctx) {
        std::cout<<"create conn succ, conn id=" << connId<<std::endl;
        if (ctx) {
            char* tmp_ctx = (char*)ctx;
            std::cout <<"conn ctx=" <<tmp_ctx <<std::endl;
            delete[] tmp_ctx;
        }
    }
    virtual void ConnCreateFail(uint64_t connId, void* ctx) {
        std::cout<<"create conn fail, conn id=" << connId<<std::endl;
    }
    virtual void CloseConnection(uint64_t connId, ConnCloseType type) { 
        std::cout<<"close conn, conn id=" << connId<<std::endl;
    }
    virtual void ConnIpChange(uint64_t connId, std::string ip, int port) {
        std::cout<<"conn id="<<connId<<",ip changed, ip="<<ip<<",port="<<port<<std::endl;
    }
};


class streamClientHandler : public StreamHandler {
private:
    XMDTransceiver* transceiver;
public:
    streamClientHandler(XMDTransceiver* t) {
        transceiver = t;
    }
    virtual void NewStream(uint64_t connId, uint16_t streamId) {
        std::cout<<"new stream, connid="<< connId<< ",stream id="<<streamId<<std::endl;
    }
    virtual void CloseStream(uint64_t connId, uint16_t streamId) { 
        std::cout<<"close stream, connid="<<connId<<",stream id="<<streamId<<std::endl;
    }
    virtual void RecvStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) { 
        uint64_t recvtime = current_ms();
        uint32_t* index = (uint32_t*)data;
        uint64_t* sendtime = (uint64_t*)(data + 4);
        
        std::string tmpStr(data, len);
        //std::cout<<"recv stream data connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<",len="<<len<<",data="<<tmpStr<<std::endl;
        std::cout<<recvtime<<":recv stream data connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<",len="<<len<<std::endl;

        std::cout<<"index="<<*index<<",sendtime="<<*sendtime<<",recvtime="<<recvtime<<",ttl="<<(recvtime - *sendtime)<<std::endl;
    }
    virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        std::cout<<current_ms()<<":send stream data succ, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
        char* tmp = (char*)ctx;
        delete[] ctx;
    }
    
    virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        std::cout<<"send stream data fail, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
        char* tmp = (char*)ctx;
        delete[] ctx;
    }
};

class SocketErrHander : public XMDSocketErrHandler {
private:
    XMDTransceiver* transceiver;

public:
    SocketErrHander(XMDTransceiver* t) {
        transceiver = t;
    }
    virtual void handle(int err_no, std::string err_reson) {
         std::cout<<"socket err " << err_no<< ",err reson=" << err_reson << std::endl;
         transceiver->stop();
    }
};


