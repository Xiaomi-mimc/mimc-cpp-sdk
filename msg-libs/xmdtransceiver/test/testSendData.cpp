#include <gtest/gtest.h>
#include "XMDTransceiver.h"
#include "common.h"
#include "fec.h"
#include<stdlib.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>



int tmpi = 0;


class DataGramHandler : public DatagramRecvHandler {
public:
    virtual void handle(char* ip, int port, char* data, uint32_t len, unsigned char packetType) {
        //std::cout<<"recv"<<std::endl;
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


class stHandler : public StreamHandler {
public:
    virtual void NewStream(uint64_t connId, uint16_t streamId) {
        std::cout<<"new stream, connid="<< connId<< ",stream id="<<streamId<<std::endl;
    }
    virtual void CloseStream(uint64_t connId, uint16_t streamId) { 
        std::cout<<"close stream, connid="<<connId<<",stream id="<<streamId<<std::endl;
    }
    virtual void RecvStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) { 
        std::string tmpStr(data, len);
        std::cout<<"recv stream data connid="<<conn_id<<",stream id="<<stream_id<<"group id="<<groupId<<",len="<<len<<",data="<<tmpStr<<std::endl;
        std::cout<<"time="<<current_ms()<<std::endl;
    }

    virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        std::cout<<"send stream data succ, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
    }
    
    virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
        std::cout<<"send stream data fail, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
    }

    virtual void sendFECStreamDataComplete(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) { 
        std::cout<<"fec stream data, connid="<<conn_id<<",stream id="<<stream_id<<",groupid="<<groupId<<std::endl;
    }
};


class SocketErrHander : public XMDSocketErrHandler {
public:
    virtual void handle(int err_no, std::string err_reson) {
         std::cout<<"socket err " << err_no<< std::endl;
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



/*
TEST(test_xmdtransceiver, test_send_immediately) {
    XMDTransceiver* transceiver11 = new XMDTransceiver(1, 45678);
    transceiver11->start();
    transceiver11->registerRecvDatagramHandler(new DataGramHandler());

    XMDTransceiver* transceiver21 = new XMDTransceiver(1, 46897);
    transceiver21->start();
    transceiver21->registerRecvDatagramHandler(new DataGramHandler());

    transceiver11->run();
    transceiver21->run();

    std::string message = "test_send_immediately";
    std::string ip = "10.231.49.218";
    int port = 46897;
    EXPECT_EQ(46897, port);
    usleep(100000);
    transceiver11->sendDatagram((char*)ip.c_str(), port, (char*)message.c_str(), message.length(), 0);

    usleep(2000000);
    std::cout<<"test="<<message.length() << std::endl;
}




TEST(test_xmdtransceiver, test_send_delay) {
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45699);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44235);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());

    transceiver->run();
    transceiver2->run();

    std::string message = "test_send_delay";
    std::string message2 = "test_send_delay2";
    std::string message3 = "test_send_delay34";
    std::string ip = "10.231.49.218";
    int port = 44235;
    EXPECT_EQ(44235, port);
    transceiver->sendDatagram((char*)ip.c_str(), port, (char*)message.c_str(), message.length(), 10);
    transceiver->sendDatagram((char*)ip.c_str(), port, (char*)message2.c_str(), message2.length(), 0);
    //usleep(5000);
    transceiver->sendDatagram((char*)ip.c_str(), port, (char*)message3.c_str(), message3.length(), 50);

    usleep(100000);
    std::string iptest;
    uint16_t porttest;
    transceiver->getLocalInfo(iptest, porttest);
    transceiver2->getLocalInfo(iptest, porttest);
    std::cout<<"ip="<<iptest<<",port="<<porttest<<std::endl;

    

    
    std::cout<<"test"<<std::endl;
}

*/





TEST(test_xmdtransceiver, test_send_ackstreamData) {
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_INFO);

    XMDTransceiver* transceiver = new XMDTransceiver(1, 45699);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44564);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());
    transceiver2->registerConnHandler(new newConn());
    transceiver2->registerStreamHandler(new stHandler());

    //transceiver->setXMDLogLevel(XMD_INFO);
    //transceiver2->setXMDLogLevel(XMD_INFO);

    transceiver->run();
    transceiver2->run();




    std::string message = "test_send_createconn";
    std::string message2 = "test_send_RTDATA";
    std::string message3 = "1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefefffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    std::string ip = "127.0.0.1";
    int port = 44564;

    int len = 10;
    char* data = new char[10];
    //transceiver->setTestPacketLoss(100);
    
    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 100, NULL);
                                   
    EXPECT_EQ(44564, port);
    usleep(2000000);


    std::string testip;
    int32_t testport;
    transceiver2->getPeerInfo(connId, testip, testport);
    XMDLoggerWrapper::instance()->debug("ip=%s, ip len=%d,port=%d", testip.c_str(), testip.length(),testport);
    std::cout<<"connid="<<connId<<",ip="<<testip<<",port="<<testport<<std::endl;

    usleep(200000);
    uint16_t streamId = transceiver->createStream(connId, ACK_STREAM, 400, false);
    std::cout<<"stream id="<<streamId<<std::endl;


    int groupid = 0;
    transceiver->setTestPacketLoss(100);
    groupid = transceiver->sendRTData(connId, streamId, (char*)message3.c_str(), message3.length(), false, P0, -1);
    std::cout<<"groupid=" <<groupid << std::endl;
    usleep(20000000);
    transceiver->setTestPacketLoss(0);
    groupid = transceiver->sendRTData(connId, streamId, (char*)message2.c_str(), message2.length(), false, P0, 1);
    std::cout<<"groupid=" <<groupid << std::endl;
    usleep(2000000);
    transceiver->closeStream(connId, streamId);
    usleep(200000);
    transceiver->closeConnection(connId);
    usleep(200000);
    transceiver->setSendBufferSize(1024 * 1024 *10);
    std::cout<<"test="<< connId <<",size="<<transceiver->getSendBufferMaxSize() << std::endl;


    transceiver2->stop();
    transceiver->stop();
    transceiver2->join();
    transceiver->join();

    //transceiver2->stop();
    delete transceiver2;
    //transceiver->stop();
    delete transceiver;


    std::cout<<"current ms ="<<current_ms()<<std::endl;


    usleep(200000);
}


/*

TEST(test_xmdtransceiver, test_rtt) {
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45699);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->setXMDLogLevel(XMD_INFO);

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44564);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());
    transceiver2->registerConnHandler(new newConn());
    transceiver2->registerStreamHandler(new stHandler());

    transceiver->run();
    transceiver2->run();

    std::string ip = "127.0.0.1";
    int port = 44564;
    std::string message = "test_send_createconn";

    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 10, NULL);

    usleep(200000);
    transceiver->sendTestRttPacket(connId, 100, 100);
    usleep(13000000);

    netStatus status =  transceiver->getNetStatus(connId);
    std::cout << "tocal packets:"<<status.totalPackets <<" rtt "<<status.rtt << " packet loss="<<status.packetLossRate<<std::endl;

    transceiver->closeConnection(connId);
    usleep(200000);
    transceiver->stop();
    transceiver2->stop();
    transceiver->join();
    transceiver2->join();
    
    delete transceiver;
    delete transceiver2;
}


*/
/*
TEST(test_xmdtransceiver, test_send_fecstreamData) {
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_DEBUG);
    
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45688);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->registerSocketErrHandler(new SocketErrHander());

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44234);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());
    transceiver2->registerConnHandler(new newConn());
    transceiver2->registerStreamHandler(new stHandler());

    transceiver->run();
    transceiver2->run();




    std::string message = "test_send_createconn";
    std::string message2 = "test_send_RTDATA";
    std::string message3 = "1234567890abcdefghijklmnopqrstuvwxyz1234561234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff风无1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff法为1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff违法未访问1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff涉非1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff法为1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff二分s1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffsfef1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffefef1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffefefffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7无1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff风无1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff法为1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff违法未访问1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff涉非1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff法为1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff二分s1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffsfef1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffefef1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffefefffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff法威风威风威1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff风无1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff法为1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff违法未访问1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff涉非1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff法为1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeff二分s1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffsfef1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffefef1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefeffefefffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";

    int tmp_msg_len = 100000;
    unsigned char* tmp_msg = new unsigned char[tmp_msg_len];
    memcpy(tmp_msg, message3.c_str(), message3.length());
    std::string ip = "127.0.0.1";
    int port = 44234;

    int len = 10;
    char* data = new char[10];
    
    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 30, NULL);
                                   
    EXPECT_EQ(44234, port);
    usleep(200000);


    std::string testip;
    int testport;
    transceiver2->getPeerInfo(connId, testip, testport);
    XMDLoggerWrapper::instance()->debug("ip=%s, ip len=%d,port=%d", testip.c_str(), testip.length(),testport);
    std::cout<<"connid="<<connId<<",ip="<<testip<<",port="<<testport<<std::endl;

    usleep(200000);
    uint16_t streamId = transceiver->createStream(connId, FEC_STREAM, 0, false);
    std::cout<<"stream id="<<streamId<<std::endl;

    uint16_t streamId2 = transceiver->createStream(connId, FEC_STREAM, 0, false);
    std::cout<<"stream id2="<<streamId2<<std::endl;

    transceiver->sendRTData(connId, streamId, (char*)tmp_msg, tmp_msg_len, NULL);
    transceiver->sendRTData(connId, streamId, (char*)tmp_msg, tmp_msg_len, NULL);
    usleep(2);
    std::cout<<"send buffer size="<<transceiver->getSendBufferMaxSize()<<std::endl;
    std::cout<<"send buffer useage="<<transceiver->getSendBufferUsageRate()<<std::endl;
    transceiver->setSendBufferSize(100);
    std::cout<<"send buffer useage="<<transceiver->getSendBufferUsageRate()<<std::endl;
    //transceiver->clearSendBuffer();
    usleep(200000);
    //transceiver->sendRTData(connId, streamId, (char*)message3.c_str(), message3.length());
    usleep(5000000);
    transceiver->closeStream(connId, streamId);
    usleep(200000);
    //transceiver->closeConnection(connId);
    usleep(100000);
    std::cout<<"test="<< connId << std::endl;


    transceiver2->stop();
    transceiver->stop();
    usleep(100000);
    delete transceiver;
    delete transceiver2;


    usleep(200000);
}
/*

TEST(test_xmdtransceiver, test_create_conn_fail) {
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_DEBUG);
    
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45488);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->registerSocketErrHandler(new SocketErrHander());

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44334);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());
    transceiver2->registerConnHandler(new newConn());
    transceiver2->registerStreamHandler(new stHandler());

    transceiver->run();
    transceiver2->run();

    transceiver->setTestPacketLoss(100);

    std::string message = "test_send_createconn";
    std::string ip = "127.0.0.1";
    int port = 44334;
    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 30, NULL);

    usleep(10000000);
    transceiver2->stop();
    transceiver->stop();
    usleep(100000);
    delete transceiver;
    delete transceiver2;
}

TEST(test_xmdtransceiver, test_send_ack_data_fail) {
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_DEBUG);
    
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45488);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->registerSocketErrHandler(new SocketErrHander());

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44334);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());
    transceiver2->registerConnHandler(new newConn());
    transceiver2->registerStreamHandler(new stHandler());

    transceiver->run();
    transceiver2->run();

    

    std::string message = "test_send_createconn";
    std::string ip = "127.0.0.1";
    int port = 44334;
    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 30, NULL);

    usleep(200000);
    uint16_t streamId = transceiver->createStream(connId, ACK_STREAM, 100, false);
    std::cout<<"stream id="<<streamId<<std::endl;

    transceiver->setTestPacketLoss(100);
    int groupid = groupid = transceiver->sendRTData(connId, streamId, (char*)message.c_str(), message.length(), false, P0, 2);
    usleep(10000000);
    transceiver2->stop();
    transceiver->stop();
    usleep(100000);
    delete transceiver;
    delete transceiver2;
}


TEST(test_xmdtransceiver, test_conn_timeout) {
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_DEBUG);
    
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45488);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->registerSocketErrHandler(new SocketErrHander());

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44334);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());
    transceiver2->registerConnHandler(new newConn());
    transceiver2->registerStreamHandler(new stHandler());

    transceiver->run();
    transceiver2->run();

    

    std::string message = "test_send_createconn";
    std::string ip = "127.0.0.1";
    int port = 44334;
    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 10, NULL);

    usleep(100000);
    transceiver->setTestPacketLoss(100);
    usleep(20000000);
    transceiver2->stop();
    transceiver->stop();
    usleep(100000);
    delete transceiver;
    delete transceiver2;
}

TEST(test_xmdtransceiver, test_ack_stream_timeout) {
    XMDLOG xmdlog_;
    XMDTransceiver::setExternalLog(&xmdlog_);
    XMDTransceiver::setXMDLogLevel(XMD_DEBUG);
    
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45488);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->registerSocketErrHandler(new SocketErrHander());

    XMDTransceiver* transceiver2 = new XMDTransceiver(1, 44334);
    transceiver2->start();
    transceiver2->registerRecvDatagramHandler(new DataGramHandler());
    transceiver2->registerConnHandler(new newConn());
    transceiver2->registerStreamHandler(new stHandler());

    transceiver->run();
    transceiver2->run();

    

    std::string message = "test_send_createconn";
    std::string ip = "127.0.0.1";
    int port = 44334;
    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 30, NULL);

    usleep(200000);
    uint16_t streamId = transceiver->createStream(connId, ACK_STREAM, 100, false);
    std::cout<<"stream id="<<streamId<<std::endl;

    //transceiver->setTestPacketLoss(100);
    int groupid = groupid = transceiver->sendRTData(connId, streamId, (char*)message.c_str(), message.length(), false, P0, 2);
    usleep(100000);
    transceiver->setTestPacketLoss(100);
    groupid = groupid = transceiver->sendRTData(connId, streamId, (char*)message.c_str(), message.length(), false, P0, 0);
    usleep(100000);
    transceiver->setTestPacketLoss(0);
    groupid = groupid = transceiver->sendRTData(connId, streamId, (char*)message.c_str(), message.length(), false, P0, 2);
    usleep(10000000);
    transceiver2->stop();
    transceiver->stop();
    usleep(100000);
    delete transceiver;
    delete transceiver2;
}

*/


/*

TEST(test_xmdtransceiver, test_with_server) {
    XMDTransceiver* transceiver = new XMDTransceiver(1, 45688);
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());


    transceiver->run();


    std::string message = "test_send_createconn";
    std::string message2 = "test_send_RTDATA";
    std::string message3 = "1234567890abcdefghijklmnopqrstuvwxyz1234567无法威风威风威风无法为违法未访问涉非法为二分ssfefefefefefffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    std::string ip = "10.231.49.218";
    int port = 44323;

    int len = 10;
    char* data = new char[10];
    
    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 0, NULL, true);
                                   
    usleep(200000);

    usleep(200000);
    uint16_t streamId = transceiver->createStream(connId, FEC_STREAM, 0);
    std::cout<<"stream id="<<streamId<<std::endl;

    transceiver->sendRTData(connId, streamId, (char*)message3.c_str(), message3.length());
    usleep(200000);
    //transceiver->sendRTData(connId, streamId, (char*)message3.c_str(), message3.length());
    usleep(120000);
    transceiver->closeStream(connId, streamId);
    usleep(200000);
    transceiver->closeConnection(connId);
    usleep(200000);
    std::cout<<"test="<< connId << std::endl;

    transceiver->stop();
    delete transceiver;

    usleep(200000);
}








/*


TEST(test_xmdtransceiver, test_with_java) {
    //XMDTransceiver* transceiver = new XMDTransceiver(1, 45678);
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new stHandler());
    transceiver->run();

    std::string message = "test_send_datagram.";
    std::string message2 = "1234567890abcdefghijklmnopqrstuvwxyzsdfef符文呃呃呃呃呃呃呃呃呃呃呃呃呃呃呃呃呃呃呃呃呃sss";
    
    std::string ip = "10.231.49.205";
    int port = 56770;

    

    usleep(200000);

    uint64_t connId = transceiver->createConnection((char*)ip.c_str(), port, 
                                   (char*)message.c_str(), message.length(), 0, NULL, true);
                                   
    std::cout<<"connid="<<connId<<std::endl;
    usleep(2000000);
    uint16_t streamId = transceiver->createStream(connId, FEC_STREAM, 0);

    
    std::cout<<"streamId="<<streamId<<std::endl;
    transceiver->sendRTData(connId, streamId, (char*)message2.c_str(), message2.length());

    usleep(200000);
    std::cout<<"send rt data"<<std::endl;
    transceiver->closeStream(connId, streamId);

    usleep(200000);
    std::cout<<"close stream"<<std::endl;
    transceiver->closeConnection(connId);
    usleep(200000);
    std::cout<<"close conn"<<std::endl;
    usleep(20000000000);
}






TEST(test_xmdtransceiver, test_fec) {
    int n = 40;
    Fec fec(n,20);
    int** matrix = new int* [n];
    int** input = new int* [n];
    int** fec_matrix = fec.get_matrix();
    for (int i = 0; i < n; i++) {
        matrix[i] = new int[n];
        input[i] = new int[n];
        for (int j = 0; j < n; j++) {
            matrix[i][j] = 0;
            input[i][j] = 0;
        }
        matrix[i][i] = 1;
    }

    bool test[n + 20];
    for (int i = 0; i < n + 20; i++) {
        test[i] = false;
    }

    srand(500);
    int t = 0;
    for (; t < n; t++) {
        while (1) {
            int tmp = rand() % (n + 20);
            //std::cout<<tmp<<std::endl;
            if (test[tmp]) {
                //std::cout<<"break="<<tmp<<std::endl;
                continue;
            }
            test[tmp] = true;
            std::cout<<tmp<<std::endl;
            //std::cout<<t<<std::endl;
            break;
        }
        
    }

    int index = 0;
    int count = 0;
    for (int i = 0; i < n + 20; i++) {
        if (!test[i]) {
            continue;
        }

        if (i < n) {
            for (int j = 0; j < n; j++) {
                input[index][j] = matrix[i][j];
            }
        } else {
            for (int j = 0; j < n; j++) {
                input[index][j] = fec_matrix[i - n][j];
            }
        }
        index++;
    }   

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout<<input[i][j]<<",";
        }
        std::cout<<std::endl;
    }

    std::cout<<current_ms()<<std::endl;
    fec.reverse_matrix(input);
    std::cout<<current_ms()<<std::endl;
}

*/



/*


TEST(TESTIP, IP) {
    const int IP_STR_LEN = 32;
    char ipStr[IP_STR_LEN];
    memset(ipStr, 0, IP_STR_LEN);
    int ip = 1765407612;
    inet_ntop(AF_INET, &ip, ipStr, IP_STR_LEN);
    std::cout<<"ip="<<ipStr<<std::endl;
}




TEST(testras, rsa) {
    const int SESSION_KEY_LEN = 128;
    RSA* rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
    unsigned char publickey[1024];
    unsigned char* tmpKey = publickey;
    int publickeyLen = i2d_RSAPublicKey(rsa, & tmpKey);
    //std::cout<<"public key="<<publickey<<",key len="<<publickeyLen<<",rsa="<<rsa<<",rsa len="<<RSA_size(rsa)<<std::endl;


    int n_len = BN_num_bytes(rsa->n);
    int e_len = BN_num_bytes(rsa->e);
    std::cout<<"nlen="<<n_len<<",elen="<<e_len<<std::endl;
    unsigned char nbignum[n_len];
    BN_bn2bin(rsa->n, nbignum);
    std::string tmpnstr((char*)nbignum, n_len);
    std::cout<<"nstr="<<tmpnstr<<std::endl;
    unsigned char ebignum[e_len];
    BN_bn2bin(rsa->e, ebignum);

    usleep(1000);

    RSA* rsa2 = RSA_new();
    rsa2->n = BN_bin2bn(nbignum, n_len, rsa2->n);
    rsa2->e = BN_bin2bn(ebignum, e_len, rsa2->e);

    ////
    std::cout<<"rsa len"<<std::endl;
    char* tmp = new char[16];
    memcpy(tmp, "asdfasdf", 8);
    RSA* tmprsa;
    tmprsa = rsa;
    int size = RSA_size(rsa);
    std::cout<<"rsa len2"<<std::endl;

    tmpKey = publickey;
    RSA* encryptRsa = d2i_RSAPublicKey(NULL, (const unsigned char**)&tmpKey, publickeyLen);
    if (NULL == encryptRsa) {
        std::cout<<"invalid rsa"<<std::endl;
    }
    ///

    unsigned char keyIn[4] = {0};
    unsigned char keyOut[SESSION_KEY_LEN];
    memcpy(keyIn, "aeee", 4); //test
    int sesisonkeyLen = RSA_public_encrypt(4, (const unsigned char*)keyIn, keyOut, rsa2, RSA_PKCS1_PADDING);
    //std::cout<<"sessionkey len="<<sesisonkeyLen<<",session key="<<keyOut<<std::endl;


    unsigned char keyIn2[SESSION_KEY_LEN] = {0};
    unsigned char keyOut2[SESSION_KEY_LEN] = {0};
    memcpy(keyOut2, keyOut, SESSION_KEY_LEN);
    //std::cout<<"session key="<<keyIn<<std::endl;
    //int len = RSA_size(tmprsa);
    int keyLen = RSA_private_decrypt(SESSION_KEY_LEN, (const unsigned char*)keyOut2, keyIn2, rsa, RSA_PKCS1_PADDING);
    std::cout<<"len="<<keyLen<<",decrypt key="<<keyIn2<<std::endl;

    RSA_free(rsa);
    //RSA_free(encryptRsa);
    RSA_free(rsa2);
<<<<<<< HEAD
}*/

