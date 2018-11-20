#ifndef MIMC_CPP_SDK_RTS_STREAM_HANDLER_H
#define MIMC_CPP_SDK_RTS_STREAM_HANDLER_H

#include <StreamHandler.h>
#include <mimc/user.h>
#include <mimc/rts_context.h>

class RtsStreamHandler : public StreamHandler {
public:
	RtsStreamHandler(User* user) {
		this->user = user;
	}
	virtual void NewStream(uint64_t connId, uint16_t streamId) {
		
	}
	virtual void CloseStream(uint64_t connId, uint16_t streamId) {
		
	}
	virtual void RecvStreamData(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, char* data, int len) {
		
		mimc::UserPacket userPacket;
		if (!userPacket.ParseFromArray(data, len)) {
			
			return;
		}
		if (!userPacket.has_uuid() || !userPacket.has_pkt_type() || !userPacket.has_resource()) {
			
			return;
		}
		

		if (userPacket.pkt_type() == mimc::BIND_RELAY_RESPONSE) {
			this->user->setLatestLegalRelayLinkStateTs(time(NULL));
			mimc::BindRelayResponse bindRelayResponse;
			if (!bindRelayResponse.ParseFromString(userPacket.payload())) {
				
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->getCurrentChats()->clear();
				this->user->resetRelayLinkState();
				return;
			}
			if (!bindRelayResponse.has_result() || !bindRelayResponse.has_internet_ip() || !bindRelayResponse.has_relay_ip() || !bindRelayResponse.has_internet_port() || !bindRelayResponse.has_relay_port()) {
				
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->getCurrentChats()->clear();
				this->user->resetRelayLinkState();
				return;
			}
			if (!bindRelayResponse.result()) {
				
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->getCurrentChats()->clear();
				this->user->resetRelayLinkState();
				return;
			}
			this->user->setBindRelayResponse(bindRelayResponse);
			this->user->setRelayLinkState(SUCC_CREATED);
			std::map<long, P2PChatSession>* currentChats = this->user->getCurrentChats();
			for (std::map<long, P2PChatSession>::iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
				const long& chatId = iter->first;
				XMDLoggerWrapper::instance()->info("In BIND_RELAY_RESPONSE, chatId is %ld", chatId);
				P2PChatSession& p2pChatSession = iter->second;
				if (p2pChatSession.getChatState() == WAIT_SEND_CREATE_REQUEST && p2pChatSession.isCreator()) {
					
					RtsSendSignal::sendCreateRequest(this->user, chatId);
				} else if (p2pChatSession.getChatState() == WAIT_CALL_ONLAUNCHED && !p2pChatSession.isCreator()) {
					p2pChatSession.setChatState(WAIT_INVITEE_RESPONSE);
					struct onLaunchedParam* param = new struct onLaunchedParam();
					param->user = this->user;
					param->chatId = chatId;
					pthread_t onLaunchedThread;
					pthread_attr_t attr;
					pthread_attr_init(&attr);
					pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
					pthread_create(&onLaunchedThread, &attr, User::onLaunched, (void *)param);
					this->user->getOnlaunchChats()->insert(std::pair<long, pthread_t>(chatId, onLaunchedThread));
					pthread_attr_destroy(&attr);
				} else if (p2pChatSession.getChatState() == WAIT_SEND_UPDATE_REQUEST) {

				}
			}
		} else if (userPacket.pkt_type() == mimc::PING_RELAY_RESPONSE) {
			mimc::PingRelayResponse pingRelayResponse;
			if (!pingRelayResponse.ParseFromString(userPacket.payload())) {
				
				return;
			}
			if (!pingRelayResponse.has_result() || !pingRelayResponse.has_internet_ip() || !pingRelayResponse.has_internet_port()) {
				
				return;
			}
			const mimc::BindRelayResponse& bindRelayResponse = this->user->getBindRelayResponse();
			if (bindRelayResponse.internet_ip() != pingRelayResponse.internet_ip() || bindRelayResponse.internet_port() != pingRelayResponse.internet_port()) {
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->resetRelayLinkState();
				RtsSendData::createRelayConn(this->user);
				std::map<long, P2PChatSession>* currentChats = this->user->getCurrentChats();
				for (std::map<long, P2PChatSession>::iterator iter = currentChats->begin(); iter != currentChats->end(); iter++) {
					P2PChatSession& chatSession = iter->second;
					chatSession.setChatState(WAIT_SEND_UPDATE_REQUEST);
					chatSession.setLatestLegalChatStateTs(time(NULL));
				}
			}
		} else if (userPacket.pkt_type() == mimc::USER_DATA_AUDIO) {
			long chatId = userPacket.chat_id();
			
			if (this->user->getCurrentChats()->count(chatId) == 0) {
				
				return;
			}
			XMDLoggerWrapper::instance()->info("In USER_DATA_AUDIO");
			const std::string& data = userPacket.payload();
			if (conn_id == this->user->getRelayConnId()) {
				this->user->getRTSCallEventHandler()->handleData(chatId, data, AUDIO, RELAY);
			} else if (conn_id == this->user->getP2PIntranetConnId(chatId)) {
				this->user->getRTSCallEventHandler()->handleData(chatId, data, AUDIO, P2P_INTRANET);
			} else if (conn_id == this->user->getP2PInternetConnId(chatId)) {
				this->user->getRTSCallEventHandler()->handleData(chatId, data, AUDIO, P2P_INTERNET);
			} else {
				return;
			}
		} else if (userPacket.pkt_type() == mimc::USER_DATA_VIDEO) {
			long chatId = userPacket.chat_id();
			
			if (this->user->getCurrentChats()->count(chatId) == 0) {
				
				return;
			}
			XMDLoggerWrapper::instance()->info("In USER_DATA_VIDEO");
			const std::string& data = userPacket.payload();
			if (conn_id == this->user->getRelayConnId()) {
				this->user->getRTSCallEventHandler()->handleData(chatId, data, VIDEO, RELAY);
			} else if (conn_id == this->user->getP2PIntranetConnId(chatId)) {
				this->user->getRTSCallEventHandler()->handleData(chatId, data, VIDEO, P2P_INTRANET);
			} else if (conn_id == this->user->getP2PInternetConnId(chatId)) {
				this->user->getRTSCallEventHandler()->handleData(chatId, data, VIDEO, P2P_INTERNET);
			} else {
				return;
			}
		}
	}

	virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
		XMDLoggerWrapper::instance()->info("RtsStreamHandler::sendStreamDataSucc, conn_id is %ld, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);
		if (ctx != NULL) {
			RtsContext* rtsContext = (RtsContext*)ctx;
			this->user->getRTSCallEventHandler()->handleSendDataSucc(rtsContext->getChatId(), groupId, rtsContext->getCtx());
			delete rtsContext;
		}
	}

	virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
		XMDLoggerWrapper::instance()->info("RtsStreamHandler::sendStreamDataFail, conn_id is %ld, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);
		if (ctx != NULL) {
			RtsContext* rtsContext = (RtsContext*)ctx;
			this->user->getRTSCallEventHandler()->handleSendDataFail(rtsContext->getChatId(), groupId, rtsContext->getCtx());
			delete rtsContext;
		}
	}
private:
	User* user;
};


#endif