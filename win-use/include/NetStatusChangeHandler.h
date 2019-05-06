#ifndef NETSTATUSCHANGEHANDLER_H
#define NETSTATUSCHANGEHANDLER_H

class NetStatusChangeHandler {
public:
    virtual void handle(uint64_t conn_id, short delay_ms, float packet_loss) { }
    virtual ~NetStatusChangeHandler() { }
};

#endif