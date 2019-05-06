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
				pthread_rwlock_wrlock(&this->user->getCallsRwlock());
				this->user->getCurrentCalls()->clear();
				pthread_rwlock_unlock(&this->user->getCallsRwlock());
				this->user->resetRelayLinkState();
				return;
			}
			if (!bindRelayResponse.has_result() || !bindRelayResponse.has_internet_ip() || !bindRelayResponse.has_relay_ip() || !bindRelayResponse.has_internet_port() || !bindRelayResponse.has_relay_port()) {
				
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				pthread_rwlock_wrlock(&this->user->getCallsRwlock());
				this->user->getCurrentCalls()->clear();
				pthread_rwlock_unlock(&this->user->getCallsRwlock());
				this->user->resetRelayLinkState();
				return;
			}
			if (!bindRelayResponse.result()) {
				
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				pthread_rwlock_wrlock(&this->user->getCallsRwlock());
				this->user->getCurrentCalls()->clear();
				pthread_rwlock_unlock(&this->user->getCallsRwlock());
				this->user->resetRelayLinkState();
				return;
			}
			this->user->setBindRelayResponse(bindRelayResponse);
			this->user->setRelayLinkState(SUCC_CREATED);

			pthread_rwlock_wrlock(&this->user->getCallsRwlock());
			std::map<uint64_t, P2PCallSession>* currentCalls = this->user->getCurrentCalls();
			for (std::map<uint64_t, P2PCallSession>::iterator iter = currentCalls->begin(); iter != currentCalls->end(); iter++) {
				const uint64_t& callId = iter->first;
				XMDLoggerWrapper::instance()->info("In BIND_RELAY_RESPONSE, relay bind succeed, callId is %llu", callId);
				P2PCallSession& p2pCallSession = iter->second;
				if (p2pCallSession.getCallState() == WAIT_SEND_CREATE_REQUEST && p2pCallSession.isCreator()) {
					RtsSendSignal::sendCreateRequest(this->user, callId);
				} else if (p2pCallSession.getCallState() == WAIT_CALL_ONLAUNCHED && !p2pCallSession.isCreator()) {
					p2pCallSession.setCallState(WAIT_INVITEE_RESPONSE);
					struct onLaunchedParam* param = new struct onLaunchedParam();
					param->user = this->user;
					param->callId = callId;
					pthread_t onLaunchedThread;
					pthread_attr_t attr;
					pthread_attr_init(&attr);
					pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
					pthread_create(&onLaunchedThread, &attr, User::onLaunched, (void *)param);
					this->user->getOnlaunchCalls()->insert(std::pair<uint64_t, pthread_t>(callId, onLaunchedThread));
					pthread_attr_destroy(&attr);
				} else if (p2pCallSession.getCallState() == WAIT_SEND_UPDATE_REQUEST) {

				}
			}

			pthread_rwlock_unlock(&this->user->getCallsRwlock());
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

				pthread_rwlock_wrlock(&this->user->getCallsRwlock());
				std::map<uint64_t, P2PCallSession>* currentCalls = this->user->getCurrentCalls();
				for (std::map<uint64_t, P2PCallSession>::iterator iter = currentCalls->begin(); iter != currentCalls->end(); iter++) {
					P2PCallSession& callSession = iter->second;
					callSession.setCallState(WAIT_SEND_UPDATE_REQUEST);
					callSession.setLatestLegalCallStateTs(time(NULL));
				}
				pthread_rwlock_unlock(&this->user->getCallsRwlock());
			}
		} else if (userPacket.pkt_type() == mimc::USER_DATA_AUDIO) {
			uint64_t callId = userPacket.call_id();
			pthread_rwlock_rdlock(&this->user->getCallsRwlock());
			if (this->user->getCurrentCalls()->count(callId) == 0) {
				pthread_rwlock_unlock(&this->user->getCallsRwlock());
				return;
			}
			XMDLoggerWrapper::instance()->info("In USER_DATA_AUDIO");
			const std::string& fromAccount = userPacket.from_app_account();
			const std::string& resource = userPacket.resource();
			const std::string& data = userPacket.payload();
			if (conn_id == this->user->getRelayConnId()) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, AUDIO, RELAY);
			} else if (conn_id == this->user->getP2PIntranetConnId(callId)) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, AUDIO, P2P_INTRANET);
			} else if (conn_id == this->user->getP2PInternetConnId(callId)) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, AUDIO, P2P_INTERNET);
			} else {
				
			}
			pthread_rwlock_unlock(&this->user->getCallsRwlock());
		} else if (userPacket.pkt_type() == mimc::USER_DATA_VIDEO) {
			uint64_t callId = userPacket.call_id();
			pthread_rwlock_rdlock(&this->user->getCallsRwlock());
			if (this->user->getCurrentCalls()->count(callId) == 0) {
				pthread_rwlock_unlock(&this->user->getCallsRwlock());
				return;
			}
			XMDLoggerWrapper::instance()->info("In USER_DATA_VIDEO");
			const std::string& fromAccount = userPacket.from_app_account();
			const std::string& resource = userPacket.resource();
			const std::string& data = userPacket.payload();
			if (conn_id == this->user->getRelayConnId()) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, VIDEO, RELAY);
			} else if (conn_id == this->user->getP2PIntranetConnId(callId)) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, VIDEO, P2P_INTRANET);
			} else if (conn_id == this->user->getP2PInternetConnId(callId)) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, VIDEO, P2P_INTERNET);
			} else {
				
			}
			pthread_rwlock_unlock(&this->user->getCallsRwlock());
		}
	}

	virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
		XMDLoggerWrapper::instance()->info("RtsStreamHandler::sendStreamDataSucc, conn_id is %llu, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);
		if (ctx != NULL) {
			RtsContext* rtsContext = (RtsContext*)ctx;
			this->user->getRTSCallEventHandler()->onSendDataSuccess(rtsContext->getCallId(), groupId, rtsContext->getCtx());
			delete rtsContext;
		}
	}

	virtual void sendStreamDataFail(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
		XMDLoggerWrapper::instance()->info("RtsStreamHandler::sendStreamDataFail, conn_id is %llu, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);
		if (ctx != NULL) {
			RtsContext* rtsContext = (RtsContext*)ctx;
			this->user->getRTSCallEventHandler()->onSendDataFailure(rtsContext->getCallId(), groupId, rtsContext->getCtx());
			delete rtsContext;
		}
	}

	virtual void sendFECStreamDataComplete(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
		XMDLoggerWrapper::instance()->info("RtsStreamHandler::sendFECStreamDataComplete, conn_id is %llu, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);
		if (ctx != NULL) {
			RtsContext* rtsContext = (RtsContext*)ctx;
			delete rtsContext;
		}
	}
private:
	User* user;
};


#endif