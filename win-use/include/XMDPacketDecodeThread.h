#ifndef PACKETDECODETHREAD_H
#define PACKETDECODETHREAD_H

#include "xmd_thread.h"
#include "XMDCommonData.h"
#include "PacketDispatcher.h"
#include "PacketDecoder.h"

class PackketDecodeThread : public XMDThread {
public:
    virtual void* process();
    PackketDecodeThread(int id, XMDCommonData* commonData, PacketDispatcher* dispatcher);
    ~PackketDecodeThread();

    void stop();

private:
    bool stopFlag_;
    int thread_id_;
    XMDCommonData* commonData_;
    PacketDispatcher* dispatcher_;
    PacketDecoder* packetDecoder_;
};

#endif //PACKETDECODETHREAD_H



