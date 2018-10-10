#include "XMDTransceiver.h"
#include <iostream>

class DataGramHandler : public DatagramRecvHandler {
public:
    virtual void handle(char* ip, int port, char* data, uint32_t len) {
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


class streamServerHandler : public StreamHandler {
private:
    XMDTransceiver* transceiver;
public:
    streamServerHandler(XMDTransceiver* t) {
        transceiver = t;
    }
    virtual void NewStream(uint64_t connId, uint16_t streamId) {
        std::cout<<"new stream, connid="<< connId<< ",stream id="<<streamId<<std::endl;
    }
    virtual void CloseStream(uint64_t connId, uint16_t streamId) { 
        std::cout<<"close stream, connid="<<connId<<",stream id="<<streamId<<std::endl;
    }
    virtual void RecvStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) { 
        std::string tmpStr(data, len);
        //std::cout<<"recv stream data connid="<<conn_id<<",stream id="<<stream_id<<",len="<<len<<",data="<<tmpStr<<std::endl;
        std::cout<<"recv stream data connid="<<conn_id<<",stream id="<<stream_id<<",len="<<len<<
std::endl;

        std::cout<<"time="<<current_ms()<<std::endl;
        transceiver->sendRTData(conn_id, stream_id, data, len);
    }
    virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        std::cout<<"send stream data succ, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
    }
    
    virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        std::cout<<"send stream data fail, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
    }
};

