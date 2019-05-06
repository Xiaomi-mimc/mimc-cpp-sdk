#ifndef STREAMHANDLER_H
#define STREAMHANDLER_H

//#include <stdint.h>

class StreamHandler {
public:
    virtual ~StreamHandler() { }
    virtual void NewStream(uint64_t connId, uint16_t streamId) { }
    virtual void CloseStream(uint64_t connId, uint16_t streamId) { }
    virtual void RecvStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) { }
    virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) { }
    virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) { }

    //just for delete ctx
    virtual void sendFECStreamDataComplete(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) { }
};


#endif //STREAMHANDLER_H