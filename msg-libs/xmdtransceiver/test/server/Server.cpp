#include "../include/XMDTransceiver.h"
#include <iostream>
#include "handler.h"

int main() {
    XMDTransceiver* transceiver = new XMDTransceiver(1, 11000);
    transceiver->start();
    transceiver->registerRecvDatagramHandler(new DataGramHandler());
    transceiver->registerConnHandler(new newConn());
    transceiver->registerStreamHandler(new streamServerHandler(transceiver));
    transceiver->setXMDLogLevel(XMD_INFO);

    transceiver->run();

    usleep(100000000);
    //transceiver->stop();
    transceiver->join();
    delete transceiver;
}

