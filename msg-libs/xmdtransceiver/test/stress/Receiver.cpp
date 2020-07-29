#include <unistd.h>
#include "XMDTransceiver.h"

using namespace std;

class DataGramHandler : public DatagramRecvHandler {
public:
    virtual void handle(char* ip, int port, char* data, uint32_t len) {
    }

};

class newConn : public ConnectionHandler {
public:
    virtual void NewConnection(uint64_t connId, char* data, int len) { 
    }

    virtual void ConnCreateSucc(uint64_t connId, void* ctx) {
    }
    virtual void ConnCreateFail(uint64_t connId, void* ctx) {
    }
    virtual void CloseConnection(uint64_t connId, ConnCloseType type) { 
    }
    virtual void ConnIpChange(uint64_t connId, std::string ip, int port) {
    }
};

class stHandler : public StreamHandler {
private:
    XMDTransceiver* transceiver_;
    uint64_t recv_count_;
public:
    stHandler(XMDTransceiver* transceiver) {
        transceiver_ = transceiver;
        recv_count_ = 0;
    }
    virtual void NewStream(uint64_t connId, uint16_t streamId) {
    }
    virtual void CloseStream(uint64_t connId, uint16_t streamId) { 
    }
    virtual void RecvStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) { 
        recv_count_++;
        if (recv_count_ % 1000 == 0) {
            std::cout << "recv data count: " << recv_count_ << std::endl;
        }
        
        transceiver_->sendRTData(conn_id, stream_id, data, len);
    }

    virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
    }
    
    virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
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
    if (argc != 2) {
        std::cout << "port needed." << std::endl;
        return 0;
    }

    uint16_t port = atoi(argv[1]);
    XMDTransceiver* transceiver = new XMDTransceiver(1, port);
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_INFO);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler(transceiver));

    transceiver->run();
    transceiver->join();

    delete transceiver;
    return 0;
}
