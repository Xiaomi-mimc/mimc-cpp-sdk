#ifndef MIMC_CPP_SDK_RTS_CONNECTIONHANDLER_H
#define MIMC_CPP_SDK_RTS_CONNECTIONHANDLER_H

#include "ConnectionHandler.h"
#include "XMDTransceiver.h"
#include "mimc/rts_send_data.h"
#include "mimc/rts_connection_info.h"
#include "mimc/user.h"
#include "mimc/error.h"
#include <algorithm>
#include "mimc/mutex_lock.h"
#include "mimc/rts_send_signal.h"

#define TEST_PACKET_DELAY_MS 50
#define TEST_PACKET_COUNT 200

class RtsConnectionHandler : public ConnectionHandler {
public:
	RtsConnectionHandler(User* user) {
		this->user = user;
	}

	//added by huanghua for p2p on 2019-03-29
	void NewConnection(uint64_t connId, char* data, int len) {

		mimc::UserPacket userPacket;
		if (!userPacket.ParseFromArray(data, len)) {
			return;
		}
		if (!userPacket.has_uuid() || !userPacket.has_pkt_type() || !userPacket.has_resource() || !userPacket.has_call_id()) {
			XMDLoggerWrapper::instance()->warn("Recv new connection req, but do not contain valid field");
			return;
		}

		uint64_t callId = userPacket.call_id();
		mimc::PKT_TYPE pktType = userPacket.pkt_type();
		XMDLoggerWrapper::instance()->info("Recv new connection req, callId:%llu, pktType:%d", callId, pktType);

    {
		RWMutexLock rwlock(&this->user->getCallsRwlock());
        rwlock.wlock();

		P2PCallSession& callSession = this->user->getCurrentCalls()->at(callId);

		if (pktType == mimc::INTRANET_CONN_REQUEST) {
			callSession.setP2PIntranetConnId(connId);
			XMDLoggerWrapper::instance()->info("Recv new INTRANET_CONN_REQUEST uuid:%llu", userPacket.uuid());
		}

		if (pktType == mimc::INTERNET_CONN_REQUEST) {
			callSession.setP2PInternetConnId(connId);
			XMDLoggerWrapper::instance()->info("Recv new INTERNET_CONN_REQUEST uuid:%llu", userPacket.uuid());
		}
    }

        this->user->getXmdTransceiver()->SetPingTimeIntervalMs(connId, 1000);
		this->user->getXmdTransceiver()->sendTestRttPacket(connId, TEST_PACKET_DELAY_MS, TEST_PACKET_COUNT);

	}

	void ConnCreateSucc(uint64_t connId, void* ctx) {
        this->user->getXmdTransceiver()->SetPingTimeIntervalMs(connId, 1000);
		this->user->setLatestLegalRelayLinkStateTs(time(NULL));
		RtsConnectionInfo* rtsConnectionInfo = (RtsConnectionInfo*)ctx;
		if (rtsConnectionInfo->getConnType() == RELAY_CONN) {
			XMDLoggerWrapper::instance()->info("Relay connection create succeed");
			int64_t ts = Utils::currentTimeMillis();
            XMDLoggerWrapper::instance()->info("relay conn timecost end time=%llu", ts);

			uint16_t streamId = this->user->getXmdTransceiver()->createStream(connId, ACK_STREAM, ACK_STREAM_WAIT_TIME_MS, false);

			XMDLoggerWrapper::instance()->info("control streamId is %d", streamId);

			this->user->setRelayControlStreamId(streamId);

            XMDLoggerWrapper::instance()->info("relay bind timecost start time=%llu", ts);
			/* if (!RtsSendData::sendBindRelayRequest(this->user)) {
				
				this->user->getXmdTransceiver()->closeConnection(connId);
				this->user->clearCurrentCallsMap();
				this->user->resetRelayLinkState();
			} */

		} else if (rtsConnectionInfo->getConnType() == INTRANET_CONN) {
			uint64_t callId = rtsConnectionInfo->getCallId();
			RWMutexLock rwlock(&this->user->getCallsRwlock());
			rwlock.wlock();
			P2PCallSession& p2pCallSession = this->user->getCurrentCalls()->at(callId);
			p2pCallSession.setP2PIntranetConnId(connId);
		} else if (rtsConnectionInfo->getConnType() == INTERNET_CONN) {
			uint64_t callId = rtsConnectionInfo->getCallId();
			RWMutexLock rwlock(&this->user->getCallsRwlock());
			rwlock.wlock();
			P2PCallSession& p2pCallSession = this->user->getCurrentCalls()->at(callId);
			p2pCallSession.setP2PInternetConnId(connId);
		} else {
			
		}
		delete rtsConnectionInfo;
        this->user->getXmdTransceiver()->sendTestRttPacket(connId, TEST_PACKET_DELAY_MS, TEST_PACKET_COUNT);
	}

	void ConnCreateFail(uint64_t connId, void* ctx) {
		this->user->setLatestLegalRelayLinkStateTs(time(NULL));
		RtsConnectionInfo* rtsConnectionInfo = (RtsConnectionInfo*)ctx;
		if (rtsConnectionInfo->getConnType() == RELAY_CONN) {
			XMDLoggerWrapper::instance()->error("Relay connection create failed");
			RtsSendSignal::sendExceptInviteResponse(user,mimc::INVALID_OPERATION, "conn relay failed");
			this->user->clearCurrentCallsMap();
			this->user->resetRelayLinkState();
			MIMCMutexLock mutexLock(&user->getAddressMutex());
			std::vector<std::string>::iterator it;
			it = find(this->user->getRelayAddresses().begin(), this->user->getRelayAddresses().end(), rtsConnectionInfo->getAddress());
			if (it != this->user->getRelayAddresses().end()) {
				this->user->getRelayAddresses().erase(it);
			}
			if (this->user->getRelayAddresses().empty()) {
				this->user->setAddressInvalid(true);
			}
		}
		delete rtsConnectionInfo;
	}

	void CloseConnection(uint64_t connId, ConnCloseType type) {
		if (type == CLOSE_NORMAL) {
			XMDLoggerWrapper::instance()->info("XMDConnection is closed normally, connId is %llu, ConnCloseType is %d", connId, type);
			return;
		}
		//连接被非正常关闭时，重置本地XMD连接及处理会话状态
		this->user->handleXMDConnClosed(connId, type);
	}
private:
		User* user;
};

#endif