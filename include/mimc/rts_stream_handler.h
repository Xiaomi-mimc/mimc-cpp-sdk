#ifndef MIMC_CPP_SDK_RTS_STREAM_HANDLER_H
#define MIMC_CPP_SDK_RTS_STREAM_HANDLER_H

#include <StreamHandler.h>
#include <XMDTransceiver.h>
#include <mimc/user.h>
#include <mimc/rts_context.h>
#include <mimc/rts_send_signal.h>
#include <mimc/rts_data.pb.h>
#include "mimc/mutex_lock.h"

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
			XMDLoggerWrapper::instance()->error("In RecvStreamData, parse failed");
			return;
		}
		if (!userPacket.has_uuid() || !userPacket.has_pkt_type() || !userPacket.has_resource()) {
			XMDLoggerWrapper::instance()->error("In RecvStreamData, missing field");
			return;
		}

		XMDLoggerWrapper::instance()->info("In RecvStreamData, conn_id is %llu, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);

		if (userPacket.pkt_type() == mimc::BIND_RELAY_RESPONSE) {
			this->user->setLatestLegalRelayLinkStateTs(time(NULL));
			mimc::BindRelayResponse bindRelayResponse;
			if (!bindRelayResponse.ParseFromString(userPacket.payload())) {
				XMDLoggerWrapper::instance()->error("In BIND_RELAY_RESPONSE, parse failed");
				RtsSendSignal::sendExceptInviteResponse(user,mimc::INVALID_OPERATION, "bindRelayResponse failed");
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->clearCurrentCallsMap();
				this->user->resetRelayLinkState();
				return;
			}
			if (!bindRelayResponse.has_result() || !bindRelayResponse.has_internet_ip() || !bindRelayResponse.has_relay_ip() || !bindRelayResponse.has_internet_port() || !bindRelayResponse.has_relay_port()) {
				XMDLoggerWrapper::instance()->error("In BIND_RELAY_RESPONSE, missing field");
				RtsSendSignal::sendExceptInviteResponse(user,mimc::INVALID_OPERATION, "bindRelayResponse failed");
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->clearCurrentCallsMap();
				this->user->resetRelayLinkState();
				return;
			}
			if (!bindRelayResponse.result()) {
				XMDLoggerWrapper::instance()->error("In BIND_RELAY_RESPONSE, result is not true");
				RtsSendSignal::sendExceptInviteResponse(user,mimc::INVALID_OPERATION, "bindRelayResponse failed");
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->clearCurrentCallsMap();
				this->user->resetRelayLinkState();
				return;
			}
			mimc::BindRelayResponse* userBindRelayResponse = new mimc::BindRelayResponse(bindRelayResponse);
			this->user->setBindRelayResponse(userBindRelayResponse);
			this->user->setRelayLinkState(SUCC_CREATED);

			int64_t ts = Utils::currentTimeMillis();
            XMDLoggerWrapper::instance()->info("relay bind timecost end time=%llu", ts);


			RWMutexLock rwlock(&this->user->getCallsRwlock());
    		rwlock.wlock();
			std::vector<uint64_t> &tmpCallVec = this->user->getCallVec();
			for (std::vector<uint64_t>::iterator iter = tmpCallVec.begin(); iter != tmpCallVec.end(); iter++) {
				const uint64_t callId = *iter;
				XMDLoggerWrapper::instance()->info("In BIND_RELAY_RESPONSE, relay bind succeed, relayIp is %s, callId is %llu", bindRelayResponse.relay_ip().c_str(), callId);
				P2PCallSession p2pCallSession;
				if (! this->user->getCallSessionNoSafe(callId, p2pCallSession)) {
				    continue;
				}
				if (p2pCallSession.getCallState() == WAIT_SEND_CREATE_REQUEST && p2pCallSession.isCreator()) {
					RtsSendSignal::sendCreateRequest(this->user, callId,p2pCallSession);
					this->user->updateCurrentCallsMapNoSafe(callId, p2pCallSession);
				} else if (p2pCallSession.getCallState() == WAIT_CALL_ONLAUNCHED && !p2pCallSession.isCreator()) {
					p2pCallSession.setCallState(WAIT_INVITEE_RESPONSE);
					this->user->updateCurrentCallsMapNoSafe(callId, p2pCallSession);
					struct onLaunchedParam* param = new struct onLaunchedParam();
					param->user = this->user;
					param->callId = callId;
					pthread_t onLaunchedThread;
					pthread_attr_t attr;
					pthread_attr_init(&attr);
					pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
					pthread_create(&onLaunchedThread, &attr, User::onLaunched, (void *)param);
					user->insertLaunchCall(callId, onLaunchedThread);
					pthread_attr_destroy(&attr);
				} else if (p2pCallSession.getCallState() == WAIT_SEND_UPDATE_REQUEST) {
					RtsSendSignal::sendUpdateRequest(this->user, callId,p2pCallSession);
					this->user->updateCurrentCallsMapNoSafe(callId,p2pCallSession);
				}
			}

		} else if (userPacket.pkt_type() == mimc::PING_RELAY_RESPONSE) {
			const mimc::BindRelayResponse* bindRelayResponse = this->user->getBindRelayResponse();
			if (!bindRelayResponse) {
				XMDLoggerWrapper::instance()->warn("bindRelayResponse is null");
				return;
			}
			mimc::PingRelayResponse pingRelayResponse;
			if (!pingRelayResponse.ParseFromString(userPacket.payload())) {
				XMDLoggerWrapper::instance()->warn("pingRelayResponse parse failed");
				return;
			}
			if (!pingRelayResponse.has_result() || !pingRelayResponse.has_internet_ip() || !pingRelayResponse.has_internet_port()) {
				XMDLoggerWrapper::instance()->warn("pingRelayResponse no field");
				return;
			}
			/* if (bindRelayResponse->internet_ip() != pingRelayResponse.internet_ip() || bindRelayResponse->internet_port() != pingRelayResponse.internet_port()) {
				XMDLoggerWrapper::instance()->info("ip or port has changed,create relaylink again");
				this->user->getXmdTransceiver()->closeConnection(conn_id);
				this->user->resetRelayLinkState();
				RtsSendData::createRelayConn(this->user);

				RWMutexLock rwlock(&this->user->getCallsRwlock());
		        rwlock.wlock();
				std::map<uint64_t, P2PCallSession>* currentCalls = this->user->getCurrentCalls();
				for (std::map<uint64_t, P2PCallSession>::iterator iter = currentCalls->begin(); iter != currentCalls->end(); iter++) {
					P2PCallSession& callSession = iter->second;
					callSession.setCallState(WAIT_SEND_UPDATE_REQUEST);
					callSession.setLatestLegalCallStateTs(time(NULL));
				}
			} */
		} else if (userPacket.pkt_type() == mimc::USER_DATA_AUDIO) {
			uint64_t callId = userPacket.call_id();
			if (this->user->countCurrentCallsMap(callId) == 0) {
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
		} else if (userPacket.pkt_type() == mimc::USER_DATA_VIDEO) {
			uint64_t callId = userPacket.call_id();
			if (this->user->countCurrentCallsMap(callId) == 0) {
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
		} else if (userPacket.pkt_type() == mimc::USER_DATA_FILE) {
			uint64_t callId = userPacket.call_id();
			if (this->user->countCurrentCallsMap(callId) == 0) {
				return;
			}
			XMDLoggerWrapper::instance()->info("In USER_DATA_FILE");
			const std::string& fromAccount = userPacket.from_app_account();
			const std::string& resource = userPacket.resource();
			const std::string& data = userPacket.payload();
			if (conn_id == this->user->getRelayConnId()) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, FILEDATA, RELAY);
			} else if (conn_id == this->user->getP2PIntranetConnId(callId)) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, FILEDATA, P2P_INTRANET);
			} else if (conn_id == this->user->getP2PInternetConnId(callId)) {
				this->user->getRTSCallEventHandler()->onData(callId, fromAccount, resource, data, FILEDATA, P2P_INTERNET);
			} else {
				
			}
		}
	}

	virtual void sendStreamDataSucc(uint64_t conn_id, uint16_t stream_id, uint32_t groupId, void* ctx) {
		//XMDLoggerWrapper::instance()->info("RtsStreamHandler::sendStreamDataSucc, conn_id is %llu, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);
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
		//XMDLoggerWrapper::instance()->info("RtsStreamHandler::sendFECStreamDataComplete, conn_id is %llu, stream_id is %d, groupId is %d", conn_id, stream_id, groupId);
		if (ctx != NULL) {
			RtsContext* rtsContext = (RtsContext*)ctx;
			delete rtsContext;
		}
	}
private:
	User* user;
};


#endif