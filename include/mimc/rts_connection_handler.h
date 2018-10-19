#ifndef MIMC_CPP_SDK_RTS_CONNECTIONHANDLER_H
#define MIMC_CPP_SDK_RTS_CONNECTIONHANDLER_H

#include <ConnectionHandler.h>
#include <mimc/rts_connection_info.h>
#include <mimc/user.h>
#include <mimc/error.h>

class RtsConnectionHandler : public ConnectionHandler {
public:
	RtsConnectionHandler(User* user) {
		this->user = user;
	}
	void ConnCreateSucc(uint64_t connId, void* ctx) {
		this->user->setLatestLegalRelayLinkStateTs(time(NULL));
		RtsConnectionInfo* rtsConnectionInfo = (RtsConnectionInfo*)ctx;
		if (rtsConnectionInfo->getConnType() == RELAY_CONN) {
			XMDLoggerWrapper::instance()->info("Relay connection create succeed");
			//创建控制流
			unsigned short streamId = this->user->getXmdTransceiver()->createStream(connId, ACK_STREAM, STREAM_TIMEOUT, ACK_STREAM_WAIT_TIME_MS, false);

			XMDLoggerWrapper::instance()->info("control streamId is %d", streamId);

			this->user->setRelayControlStreamId(streamId);

			if (!RtsSendData::sendBindRelayRequest(this->user)) {
				
				this->user->getXmdTransceiver()->closeConnection(connId);
				this->user->getCurrentChats()->clear();
				this->user->resetRelayLinkState();
			}
		} else if (rtsConnectionInfo->getConnType() == INTRANET_CONN) {
			
			long chatId = rtsConnectionInfo->getChatId();
			P2PChatSession& p2pChatSession = this->user->getCurrentChats()->at(chatId);
			p2pChatSession.setP2PIntranetConnId(connId);
		} else if (rtsConnectionInfo->getConnType() == INTERNET_CONN) {
			
			long chatId = rtsConnectionInfo->getChatId();
			P2PChatSession& p2pChatSession = this->user->getCurrentChats()->at(chatId);
			p2pChatSession.setP2PInternetConnId(connId);
		} else {
			
		}
		delete rtsConnectionInfo;
	}

	void ConnCreateFail(uint64_t connId, void* ctx) {
		this->user->setLatestLegalRelayLinkStateTs(time(NULL));
		RtsConnectionInfo* rtsConnectionInfo = (RtsConnectionInfo*)ctx;
		if (rtsConnectionInfo->getConnType() == RELAY_CONN) {
			XMDLoggerWrapper::instance()->error("Relay connection create failed");
			this->user->getXmdTransceiver()->closeConnection(connId);
			this->user->getCurrentChats()->clear();
			this->user->resetRelayLinkState();
		}
		delete rtsConnectionInfo;
	}

	void CloseConnection(uint64_t connId, ConnCloseType type) {
		if (type == CLOSE_NORMAL) {
			XMDLoggerWrapper::instance()->info("XMDConnection is closed normally, connId is %ld, ConnCloseType is %d", connId, type);
			return;
		}
		//连接被关闭时，重置本地XMD连接及处理会话状态
		this->user->handleXMDConnClosed(connId, type);
	}
private:
		User* user;
};

#endif