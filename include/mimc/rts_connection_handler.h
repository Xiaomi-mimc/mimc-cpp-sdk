#ifndef MIMC_CPP_SDK_RTS_CONNECTIONHANDLER_H
#define MIMC_CPP_SDK_RTS_CONNECTIONHANDLER_H

#include <ConnectionHandler.h>
#include <mimc/rts_connection_info.h>
#include <mimc/user.h>
#include <mimc/error.h>
#include <algorithm>

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
			unsigned short streamId = this->user->getXmdTransceiver()->createStream(connId, ACK_STREAM, ACK_STREAM_WAIT_TIME_MS, false);

			XMDLoggerWrapper::instance()->info("control streamId is %d", streamId);

			this->user->setRelayControlStreamId(streamId);

			if (!RtsSendData::sendBindRelayRequest(this->user)) {
				
				this->user->getXmdTransceiver()->closeConnection(connId);
				pthread_rwlock_wrlock(&this->user->getChatsRwlock());
				this->user->getCurrentChats()->clear();
				pthread_rwlock_unlock(&this->user->getChatsRwlock());
				this->user->resetRelayLinkState();
			}
		} else if (rtsConnectionInfo->getConnType() == INTRANET_CONN) {
			
			long chatId = rtsConnectionInfo->getChatId();
			pthread_rwlock_wrlock(&this->user->getChatsRwlock());
			P2PChatSession& p2pChatSession = this->user->getCurrentChats()->at(chatId);
			p2pChatSession.setP2PIntranetConnId(connId);
			pthread_rwlock_unlock(&this->user->getChatsRwlock());
		} else if (rtsConnectionInfo->getConnType() == INTERNET_CONN) {
			
			long chatId = rtsConnectionInfo->getChatId();
			pthread_rwlock_wrlock(&this->user->getChatsRwlock());
			P2PChatSession& p2pChatSession = this->user->getCurrentChats()->at(chatId);
			p2pChatSession.setP2PInternetConnId(connId);
			pthread_rwlock_unlock(&this->user->getChatsRwlock());
		} else {
			
		}
		delete rtsConnectionInfo;
	}

	void ConnCreateFail(uint64_t connId, void* ctx) {
		this->user->setLatestLegalRelayLinkStateTs(time(NULL));
		RtsConnectionInfo* rtsConnectionInfo = (RtsConnectionInfo*)ctx;
		if (rtsConnectionInfo->getConnType() == RELAY_CONN) {
			XMDLoggerWrapper::instance()->error("Relay connection create failed");

			pthread_rwlock_wrlock(&this->user->getChatsRwlock());
			this->user->getCurrentChats()->clear();
			pthread_rwlock_unlock(&this->user->getChatsRwlock());
			this->user->resetRelayLinkState();
			pthread_mutex_lock(&user->getAddressMutex());
			std::vector<std::string>::iterator it;
			it = find(this->user->getRelayAddresses().begin(), this->user->getRelayAddresses().end(), rtsConnectionInfo->getAddress());
			if (it != this->user->getRelayAddresses().end()) {
				this->user->getRelayAddresses().erase(it);
			}
			if (this->user->getRelayAddresses().empty()) {
				this->user->setAddressInvalid(true);
			}
			pthread_mutex_unlock(&user->getAddressMutex());
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