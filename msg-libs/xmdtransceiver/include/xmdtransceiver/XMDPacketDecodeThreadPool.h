#ifndef DECODETHREADPOOL_H
#define DECODETHREADPOOL_H
#include <vector>
#include "XMDPacketDecodeThread.h"

class XMDPacketDecodeThreadPool {
private:
    std::vector<PackketDecodeThread*> decode_thread_pool_;
    int pool_size_;
public:
    XMDPacketDecodeThreadPool(int pool_size, XMDCommonData* commonData, PacketDispatcher* dispatcher);
    ~XMDPacketDecodeThreadPool();
    void run();
    void stop();
    void join();
};

#endif //DECODETHREADPOOL_H

