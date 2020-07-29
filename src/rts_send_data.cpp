#include "mimc/rts_send_data.h"
#include "mimc/user.h"
#include "mimc/rts_connection_info.h"
#include "mimc/utils.h"
#include "mimc/p2p_callsession.h"
#include "XMDTransceiver.h"
#include <cstdlib>
#include "mimc/mutex_lock.h"
#include "mimc/error.h"


uint64_t RtsSendData::createRelayConn(User* user) {
	if (user->getRelayConnId() != 0) {
		return user->getRelayConnId();
	}
	std::string relayAddress;
#ifndef STAGING
	srand(Utils::currentTimeMicros());

    {
	MIMCMutexLock mutexLock(&user->getAddressMutex());
	std::vector<std::string>& relayAddresses = user->getRelayAddresses();

	if (!relayAddresses.empty()) {
		relayAddress = relayAddresses.at(rand()%(relayAddresses.size()));
	} else {
		struct hostent *pHostEnt = NULL;
		pHostEnt = gethostbyname(user->getRelayDomain().c_str());
		struct sockaddr_in dest_addr;
		dest_addr.sin_addr.s_addr = *((unsigned long *)pHostEnt->h_addr_list[0]);
		relayAddress.append(inet_ntoa(dest_addr.sin_addr));
		relayAddress.append(":");
		relayAddress.append(Utils::int2str(RELAY_PORT));

		pthread_t fetchThread;
        pthread_create(&fetchThread, NULL, fetchServerAddr, (void*)user);
	}
	}

	int pos = relayAddress.find(":");
	if (pos == std::string::npos) {
		return 0;
	}
	std::string relayIp = relayAddress.substr(0, pos);
	int relayPort = atoi(relayAddress.substr(pos+1).c_str());
#else
	std::string relayIp = "10.38.162.142";
	int relayPort = 6777;
	char relayPortStr[5];
	Utils::ltoa(relayPort, relayPortStr);
	relayAddress = relayIp + ":" + relayPortStr;
#endif
	/* mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(mimc::RELAY_CONN_REQUEST);
	userPacket.set_region_bucket(user->getRegionBucket());

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size); */

	std::string localIp;
	uint16_t localPort;
	if (user->getXmdTransceiver()->getLocalInfo(localIp, localPort) < 0) {
		
		return 0;
	}
	mimc::BindRelayRequest bindRelayRequest;
	bindRelayRequest.set_uuid(user->getUuid());
	bindRelayRequest.set_resource(user->getResource());
	bindRelayRequest.set_intranet_ip(localIp);
	bindRelayRequest.set_intranet_port(localPort);
	bindRelayRequest.set_token(user->getToken());
	mimc::StreamConfig* audioStreamConfigPb = bindRelayRequest.mutable_audio_stream_default_config();
	const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
	mimc::STREAM_STRATEGY streamStrategy = mimc::ACK_STRATEGY;
	if (audioStreamConfig.getType() == FEC_TYPE) {
		streamStrategy = mimc::FEC_STRATEGY;
	}
	audioStreamConfigPb->set_stream_strategy(streamStrategy);
	audioStreamConfigPb->set_ack_stream_wait_time_ms(audioStreamConfig.getAckStreamWaitTimeMs());
	audioStreamConfigPb->set_stream_is_encrypt(audioStreamConfig.getEncrypt());
	audioStreamConfigPb->set_resend_count(10);
	mimc::StreamConfig* videoStreamConfigPb = bindRelayRequest.mutable_video_stream_default_config();
	const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
	streamStrategy = mimc::FEC_STRATEGY;
	if (videoStreamConfig.getType() == ACK_TYPE) {
		streamStrategy = mimc::ACK_STRATEGY;
	}
	videoStreamConfigPb->set_stream_strategy(streamStrategy);
	videoStreamConfigPb->set_ack_stream_wait_time_ms(videoStreamConfig.getAckStreamWaitTimeMs());
	videoStreamConfigPb->set_stream_is_encrypt(videoStreamConfig.getEncrypt());
	videoStreamConfigPb->set_resend_count(10);
	mimc::StreamConfig* fileStreamConfigPb = bindRelayRequest.mutable_file_stream_default_config();
	fileStreamConfigPb->set_stream_strategy(mimc::ACK_STRATEGY);
	fileStreamConfigPb->set_ack_stream_wait_time_ms(ACK_STREAM_WAIT_TIME_MS);
	fileStreamConfigPb->set_stream_is_encrypt(false);
	fileStreamConfigPb->set_resend_count(10);

	std::string userPacketPayload = bindRelayRequest.SerializeAsString();
	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_payload(userPacketPayload);
	userPacket.set_pkt_type(mimc::BIND_RELAY_REQUEST);
	userPacket.set_region_bucket(user->getRegionBucket());

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	XMDLoggerWrapper::instance()->info("In createRelayConn, relayIp is %s", relayIp.c_str());

    int64_t ts = Utils::currentTimeMillis();
    XMDLoggerWrapper::instance()->info("relay conn timecost begin time=%llu", ts);
	uint64_t relayConnId = user->getXmdTransceiver()->createConnection((char *)relayIp.c_str(), relayPort, messageBytes, message_size, XMD_TRAN_TIMEOUT, new RtsConnectionInfo(relayAddress, RELAY_CONN));
	if (relayConnId == 0) {
		delete[] messageBytes;
		return 0;
	}

	delete[] messageBytes;
	user->setRelayConnId(relayConnId);
	user->setRelayLinkState(BEING_CREATED);
	user->setLatestLegalRelayLinkStateTs(time(NULL));
	return relayConnId;
}

bool RtsSendData::sendBindRelayRequest(User* user) {
	std::string localIp;
	uint16_t localPort;
	if (user->getXmdTransceiver()->getLocalInfo(localIp, localPort) < 0) {
		
		return false;
	}
	mimc::BindRelayRequest bindRelayRequest;
	bindRelayRequest.set_uuid(user->getUuid());
	bindRelayRequest.set_resource(user->getResource());
	bindRelayRequest.set_intranet_ip(localIp);
	bindRelayRequest.set_intranet_port(localPort);
	bindRelayRequest.set_token(user->getToken());
	mimc::StreamConfig* audioStreamConfigPb = bindRelayRequest.mutable_audio_stream_default_config();
	const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
	mimc::STREAM_STRATEGY streamStrategy = mimc::ACK_STRATEGY;
	if (audioStreamConfig.getType() == FEC_TYPE) {
		streamStrategy = mimc::FEC_STRATEGY;
	}
	audioStreamConfigPb->set_stream_strategy(streamStrategy);
	audioStreamConfigPb->set_ack_stream_wait_time_ms(audioStreamConfig.getAckStreamWaitTimeMs());
	audioStreamConfigPb->set_stream_is_encrypt(audioStreamConfig.getEncrypt());
	audioStreamConfigPb->set_resend_count(10);
	mimc::StreamConfig* videoStreamConfigPb = bindRelayRequest.mutable_video_stream_default_config();
	const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
	streamStrategy = mimc::FEC_STRATEGY;
	if (videoStreamConfig.getType() == ACK_TYPE) {
		streamStrategy = mimc::ACK_STRATEGY;
	}
	videoStreamConfigPb->set_stream_strategy(streamStrategy);
	videoStreamConfigPb->set_ack_stream_wait_time_ms(videoStreamConfig.getAckStreamWaitTimeMs());
	videoStreamConfigPb->set_stream_is_encrypt(videoStreamConfig.getEncrypt());
	videoStreamConfigPb->set_resend_count(10);
	mimc::StreamConfig* fileStreamConfigPb = bindRelayRequest.mutable_file_stream_default_config();
	fileStreamConfigPb->set_stream_strategy(mimc::ACK_STRATEGY);
	fileStreamConfigPb->set_ack_stream_wait_time_ms(ACK_STREAM_WAIT_TIME_MS);
	fileStreamConfigPb->set_stream_is_encrypt(false);
	fileStreamConfigPb->set_resend_count(10);

	int userPacketPayloadSize = bindRelayRequest.ByteSize();
	char* userPacketPayload = new char[userPacketPayloadSize];
	memset(userPacketPayload, 0, userPacketPayloadSize);
	bindRelayRequest.SerializeToArray(userPacketPayload, userPacketPayloadSize);

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_payload(userPacketPayload, userPacketPayloadSize);
	userPacket.set_pkt_type(mimc::BIND_RELAY_REQUEST);
	userPacket.set_region_bucket(user->getRegionBucket());

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	int ret = user->getXmdTransceiver()->sendRTData(user->getRelayConnId(), user->getRelayControlStreamId(), messageBytes, message_size, false, P1, 10);
	delete[] userPacketPayload;
	delete[] messageBytes;

	if (ret < 0) {
		XMDLoggerWrapper::instance()->warn("In sendBindRelayRequest, sendRTData failed");
		return false;
	}
	XMDLoggerWrapper::instance()->info("In sendBindRelayRequest, sendRTData succeed");
	return true;
}

bool RtsSendData::sendPingRelayRequestByCallId(User* user,uint64_t callId)
{
	mimc::PingRelayRequest pingRelayRequest;
	pingRelayRequest.set_uuid(user->getUuid());
	pingRelayRequest.set_resource(user->getResource());
	pingRelayRequest.set_calltype(mimc::SINGLE_CALL);

	int userPacketPayloadSize = pingRelayRequest.ByteSize();
	char* userPacketPayload = new char[userPacketPayloadSize];
	memset(userPacketPayload, 0, userPacketPayloadSize);
	pingRelayRequest.SerializeToArray(userPacketPayload, userPacketPayloadSize);

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_payload(userPacketPayload, userPacketPayloadSize);
	userPacket.set_pkt_type(mimc::PING_RELAY_REQUEST);
	userPacket.set_region_bucket(user->getRegionBucket());
	userPacket.set_call_id(callId);

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	int ret = 0;
	ret = user->getXmdTransceiver()->sendRTData(user->getRelayConnId(), user->getRelayControlStreamId(), messageBytes, message_size, true, P1, XND_TRAN_ACK_STREAM_RESEND_COUNT, NULL); // -1
	delete[] userPacketPayload;
	delete[] messageBytes;
	if (ret < 0) {
		XMDLoggerWrapper::instance()->error("xmd sendRTData failed");
		return false;
	}
	
	return true;
}

bool RtsSendData::sendPingRelayRequest(User* user) {
	if (user->currentCallsMapSize() == 0 || user->getRelayLinkState() != SUCC_CREATED) {
		return false;
	}

	RWMutexLock rwlock(&user->getCallsRwlock());
    rwlock.rlock();
	std::vector<uint64_t> &tmpCallVec = user->getCallVec();
	for(auto iter = tmpCallVec.begin();iter != tmpCallVec.end();)
	{
		uint64_t callId = *iter;
		P2PCallSession callSession;
        if (! user->getCallSessionNoSafe(callId, callSession)) {
            iter++;
            continue;
        }

		const CallState& callState = callSession.getCallState();
		if (callState == RUNNING) {
            XMDLoggerWrapper::instance()->info("In sendPingRelayRequest, callId %llu state RUNNING ping relay", callId);
			if(!RtsSendData::sendPingRelayRequestByCallId(user,callId)){
				XMDLoggerWrapper::instance()->error("sendPingRelayRequestByCallId failed");
				return false;
			}
		}

		iter++;
	}

	return true;
}

int RtsSendData::sendRtsDataByRelay(User* user, uint64_t callId, const std::string& data, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
	int dataId = -1;
	uint64_t relayConnId = user->getRelayConnId();
	if (relayConnId == 0) {
		XMDLoggerWrapper::instance()->error("relayConnId is 0");
		return MimcError::RELAY_CONNID_ERROR;
	}
	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_from_app_account(user->getAppAccount());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(pktType);
	userPacket.set_payload(std::string(data));
	userPacket.set_call_id(callId);
	userPacket.set_region_bucket(user->getRegionBucket());

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	XMDTransceiver* xmdTransceiver = user->getXmdTransceiver();
	if (pktType == mimc::USER_DATA_AUDIO) {
		if (user->getRelayAudioStreamId() == 0) {
			const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
			StreamType streamType = ACK_STREAM;
			if (audioStreamConfig.getType() == FEC_TYPE) {
				streamType = FEC_STREAM;
			}
			user->setRelayAudioStreamId(xmdTransceiver->createStream(relayConnId, streamType, audioStreamConfig.getAckStreamWaitTimeMs(), audioStreamConfig.getEncrypt()));
			XMDLoggerWrapper::instance()->info("audio streamId is %d", user->getRelayAudioStreamId());
		}
		if (user->getRelayAudioStreamId() != 0) {
			dataId = xmdTransceiver->sendRTData(relayConnId, user->getRelayAudioStreamId(), messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		} else {
			XMDLoggerWrapper::instance()->error("send audio error, audio streamId is %d", user->getRelayAudioStreamId());
		}
	} else if (pktType == mimc::USER_DATA_VIDEO) {
		//XMDLoggerWrapper::instance()->info("send one video pkt");
		if (user->getRelayVideoStreamId() == 0) {
			const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
			StreamType streamType = FEC_STREAM;
			if (videoStreamConfig.getType() == ACK_TYPE) {
				streamType = ACK_STREAM;
			}
			user->setRelayVideoStreamId(xmdTransceiver->createStream(relayConnId, streamType, videoStreamConfig.getAckStreamWaitTimeMs(), videoStreamConfig.getEncrypt()));
			XMDLoggerWrapper::instance()->info("video streamId is %d", user->getRelayVideoStreamId());
		}
		if (user->getRelayVideoStreamId() != 0) {
			dataId = xmdTransceiver->sendRTData(relayConnId, user->getRelayVideoStreamId(), messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}else {
			XMDLoggerWrapper::instance()->error("send video error, video streamId is %d", user->getRelayVideoStreamId());
		}
	} else if (pktType == mimc::USER_DATA_FILE) {
		//XMDLoggerWrapper::instance()->info("send one file pkt");
		if (user->getRelayFileStreamId() == 0) {
			user->setRelayFileStreamId(xmdTransceiver->createStream(relayConnId, ACK_STREAM, ACK_STREAM_WAIT_TIME_MS, false));
			XMDLoggerWrapper::instance()->info("file streamId is %d", user->getRelayFileStreamId());
		}
		if (user->getRelayFileStreamId() != 0) {
			dataId = xmdTransceiver->sendRTData(relayConnId, user->getRelayFileStreamId(), messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}else {
			XMDLoggerWrapper::instance()->error("send file error, file streamId is %d", user->getRelayFileStreamId());
		}
	}

	if(dataId < 0){
		XMDLoggerWrapper::instance()->error("sendRTData error,pkttype is %d",pktType);
	}

	delete[] messageBytes;
	XMDLoggerWrapper::instance()->debug("sendRtsDataByRelay, return:%d, uuid:%llu, callId:%llu, connId:%llu, pktType:%d", dataId, user->getUuid(), callId, relayConnId, pktType);

	return dataId;
}

int RtsSendData::sendRtsDataByRelay(User* user, uint64_t callId, const char* data, const unsigned int dataSize, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
    int dataId = -1;
    uint64_t relayConnId = user->getRelayConnId();
    if (relayConnId == 0) {
        XMDLoggerWrapper::instance()->error("relayConnId is 0");
        return dataId;
    }
    mimc::UserPacket userPacket;
    userPacket.set_uuid(user->getUuid());
    userPacket.set_from_app_account(user->getAppAccount());
    userPacket.set_resource(user->getResource());
    userPacket.set_pkt_type(pktType);
    userPacket.set_payload(data, dataSize);
    userPacket.set_call_id(callId);
    userPacket.set_region_bucket(user->getRegionBucket());

    int message_size = userPacket.ByteSize();
    char* messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    userPacket.SerializeToArray(messageBytes, message_size);

    uint16_t streamId = 0;

    XMDTransceiver* xmdTransceiver = user->getXmdTransceiver();
    if (pktType == mimc::USER_DATA_AUDIO) {
        streamId = user->getRelayAudioStreamId();
        if (streamId == 0) {
            const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
            StreamType streamType = ACK_STREAM;
            if (audioStreamConfig.getType() == FEC_TYPE) {
                streamType = FEC_STREAM;
            }
            streamId = xmdTransceiver->createStream(relayConnId, streamType, audioStreamConfig.getAckStreamWaitTimeMs(), audioStreamConfig.getEncrypt());
            user->setRelayAudioStreamId(streamId);
            XMDLoggerWrapper::instance()->info("audio streamId is %d", streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(relayConnId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        } else {
            XMDLoggerWrapper::instance()->error("send audio error, audio streamId is %d", streamId);
        }
    } else if (pktType == mimc::USER_DATA_VIDEO) {
        streamId = user->getRelayVideoStreamId();
        if (streamId == 0) {
            const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
            StreamType streamType = FEC_STREAM;
            if (videoStreamConfig.getType() == ACK_TYPE) {
                streamType = ACK_STREAM;
            }
            streamId = xmdTransceiver->createStream(relayConnId, streamType, videoStreamConfig.getAckStreamWaitTimeMs(), videoStreamConfig.getEncrypt());
            user->setRelayVideoStreamId(streamId);
            XMDLoggerWrapper::instance()->info("video streamId is %d", streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(relayConnId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }else {
            XMDLoggerWrapper::instance()->error("send video error, video streamId is %d", streamId);
        }
    } else if (pktType == mimc::USER_DATA_FILE) {
        streamId = user->getRelayFileStreamId();
        if (streamId == 0) {
            streamId = xmdTransceiver->createStream(relayConnId, ACK_STREAM, ACK_STREAM_WAIT_TIME_MS, false);
            user->setRelayFileStreamId(streamId);
            XMDLoggerWrapper::instance()->info("file streamId is %d", streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(relayConnId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }else {
            XMDLoggerWrapper::instance()->error("send file error, file streamId is %d", streamId);
        }
    }

	if(dataId < 0){
		XMDLoggerWrapper::instance()->error("sendRTData error,pkttype is file");
	}

    delete[] messageBytes;

    if((dataId >= 0) && (dataId % 100  < 5))
        XMDLoggerWrapper::instance()->info("sendRtsDataByRelay, dataId:%d, uuid:%llu, callId:%llu, connId:%llu, streamId:%u, pktType:%d", dataId, user->getUuid(), callId, relayConnId, streamId, pktType);

    return dataId;
}

//added by huanghua on 2019-04-11 for p2p
int RtsSendData::sendRtsDataByP2PIntranet(User* user, uint64_t callId, const std::string& data, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
	int dataId = -1;
	P2PCallSession& callSession = user->getCurrentCalls()->at(callId);

	if (callSession.isP2PIntranetOK() == false){
		return dataId;
	}

	uint64_t connId = callSession.getP2PIntranetConnId();

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_from_app_account(user->getAppAccount());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(pktType);
	userPacket.set_payload(std::string(data));
	userPacket.set_call_id(callId);
	userPacket.set_region_bucket(user->getRegionBucket());

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	XMDTransceiver* xmdTransceiver = user->getXmdTransceiver();
	if (pktType == mimc::USER_DATA_AUDIO) {
		uint16_t audioStreamId = callSession.getP2PIntranetAudioStreamId();
		if (audioStreamId == 0) {
			const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
			StreamType streamType = ACK_STREAM;
			if (audioStreamConfig.getType() == FEC_TYPE) {
				streamType = FEC_STREAM;
			}
			audioStreamId = xmdTransceiver->createStream(connId, streamType, audioStreamConfig.getAckStreamWaitTimeMs(), audioStreamConfig.getEncrypt());
			callSession.setP2PIntranetAudioStreamId(audioStreamId);
			XMDLoggerWrapper::instance()->info("audio streamId is %d", audioStreamId);
		}
		if (audioStreamId!= 0) {
			dataId = xmdTransceiver->sendRTData(connId, audioStreamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}
	} else if (pktType == mimc::USER_DATA_VIDEO) {
		uint16_t videoStreamId = callSession.getP2PIntranetVideoStreamId();
		if (videoStreamId == 0) {
			const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
			StreamType streamType = FEC_STREAM;
			if (videoStreamConfig.getType() == ACK_TYPE) {
				streamType = ACK_STREAM;
			}
			videoStreamId = xmdTransceiver->createStream(connId, streamType, videoStreamConfig.getAckStreamWaitTimeMs(), videoStreamConfig.getEncrypt());
			callSession.setP2PIntranetVideoStreamId(videoStreamId);
			XMDLoggerWrapper::instance()->info("video streamId is %d", videoStreamId);
		}
		if (videoStreamId != 0) {
			dataId = xmdTransceiver->sendRTData(connId, videoStreamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}
	} else if (pktType == mimc::USER_DATA_FILE) {
		uint16_t fileStreamId = callSession.getP2PIntranetFileStreamId();
		if (fileStreamId == 0) {
			StreamType streamType = ACK_STREAM;
			fileStreamId = xmdTransceiver->createStream(connId, streamType, ACK_STREAM_WAIT_TIME_MS, false);
			callSession.setP2PIntranetFileStreamId(fileStreamId);
			XMDLoggerWrapper::instance()->info("file streamId is %d", fileStreamId);
		}
		if (fileStreamId != 0) {
			dataId = xmdTransceiver->sendRTData(connId, fileStreamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}
	}

	delete[] messageBytes;

	XMDLoggerWrapper::instance()->debug("sendRtsDataByP2PIntranet, return:%d, uuid:%llu, callId:%llu, connId:%llu, pktType:%d", dataId, user->getUuid(), callId, connId, pktType);

	return dataId;
}

int RtsSendData::sendRtsDataByP2PIntranet(User* user, uint64_t callId, const char* data, const unsigned int dataSize, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
    int dataId = -1;
    P2PCallSession& callSession = user->getCurrentCalls()->at(callId);

    if (callSession.isP2PIntranetOK() == false)
    {
		return dataId;
	}

    uint64_t connId = callSession.getP2PIntranetConnId();

    mimc::UserPacket userPacket;
    userPacket.set_uuid(user->getUuid());
    userPacket.set_from_app_account(user->getAppAccount());
    userPacket.set_resource(user->getResource());
    userPacket.set_pkt_type(pktType);
    userPacket.set_payload(data, dataSize);
    userPacket.set_call_id(callId);
    userPacket.set_region_bucket(user->getRegionBucket());

    int message_size = userPacket.ByteSize();
    char* messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    userPacket.SerializeToArray(messageBytes, message_size);

    uint16_t streamId = 0;

    XMDTransceiver* xmdTransceiver = user->getXmdTransceiver();
    if (pktType == mimc::USER_DATA_AUDIO) {
        streamId = callSession.getP2PIntranetAudioStreamId();
        if (streamId == 0) {
            const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
            StreamType streamType = ACK_STREAM;
            if (audioStreamConfig.getType() == FEC_TYPE) {
                streamType = FEC_STREAM;
            }
            streamId = xmdTransceiver->createStream(connId, streamType, audioStreamConfig.getAckStreamWaitTimeMs(), audioStreamConfig.getEncrypt());
            callSession.setP2PIntranetAudioStreamId(streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(connId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }
    } else if (pktType == mimc::USER_DATA_VIDEO) {
        streamId = callSession.getP2PIntranetVideoStreamId();
        if (streamId == 0) {
            const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
            StreamType streamType = FEC_STREAM;
            if (videoStreamConfig.getType() == ACK_TYPE) {
                streamType = ACK_STREAM;
            }
            streamId = xmdTransceiver->createStream(connId, streamType, videoStreamConfig.getAckStreamWaitTimeMs(), videoStreamConfig.getEncrypt());
            callSession.setP2PIntranetVideoStreamId(streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(connId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }
    } else if (pktType == mimc::USER_DATA_FILE) {
        streamId = callSession.getP2PIntranetFileStreamId();
        if (streamId == 0) {
            StreamType streamType = ACK_STREAM;
            streamId = xmdTransceiver->createStream(connId, streamType, ACK_STREAM_WAIT_TIME_MS, false);
            callSession.setP2PIntranetFileStreamId(streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(connId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }
    }

    delete[] messageBytes;

    if((dataId >= 0) && (dataId % 100  < 5))
        XMDLoggerWrapper::instance()->info("sendRtsDataByP2PIntranet, dataId:%d, uuid:%llu, callId:%llu, connId:%llu, streamId:%u, pktType:%d", dataId, user->getUuid(), callId, connId, streamId, pktType);

    return dataId;
}

//added by huanghua on 2019-04-11 for p2p
int RtsSendData::sendRtsDataByP2PInternet(User* user, uint64_t callId, const std::string& data, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
	int dataId = -1;

	P2PCallSession& callSession = user->getCurrentCalls()->at(callId);

	if (callSession.isP2PInternetOK() == false){
		return dataId;
	}

	uint64_t connId = callSession.getP2PInternetConnId();

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_from_app_account(user->getAppAccount());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(pktType);
	userPacket.set_payload(std::string(data));
	userPacket.set_call_id(callId);
	userPacket.set_region_bucket(user->getRegionBucket());

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	XMDTransceiver* xmdTransceiver = user->getXmdTransceiver();
	if (pktType == mimc::USER_DATA_AUDIO) {
		uint16_t audioStreamId = callSession.getP2PInternetAudioStreamId();
		if (audioStreamId == 0) {
			const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
			StreamType streamType = ACK_STREAM;
			if (audioStreamConfig.getType() == FEC_TYPE) {
				streamType = FEC_STREAM;
			}
			audioStreamId = xmdTransceiver->createStream(connId, streamType, audioStreamConfig.getAckStreamWaitTimeMs(), audioStreamConfig.getEncrypt());
			callSession.setP2PInternetAudioStreamId(audioStreamId);
			XMDLoggerWrapper::instance()->info("audio streamId is %d", audioStreamId);
		}
		if (audioStreamId!= 0) {
			dataId = xmdTransceiver->sendRTData(connId, audioStreamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}
	} else if (pktType == mimc::USER_DATA_VIDEO) {
		uint16_t videoStreamId = callSession.getP2PInternetVideoStreamId();
		if (videoStreamId == 0) {
			const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
			StreamType streamType = FEC_STREAM;
			if (videoStreamConfig.getType() == ACK_TYPE) {
				streamType = ACK_STREAM;
			}
			videoStreamId = xmdTransceiver->createStream(connId, streamType, videoStreamConfig.getAckStreamWaitTimeMs(), videoStreamConfig.getEncrypt());
			callSession.setP2PInternetVideoStreamId(videoStreamId);
			XMDLoggerWrapper::instance()->info("video streamId is %d", videoStreamId);
		}
		if (videoStreamId != 0) {
			dataId = xmdTransceiver->sendRTData(connId, videoStreamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}
	} else if (pktType == mimc::USER_DATA_FILE) {
		uint16_t fileStreamId = callSession.getP2PInternetFileStreamId();
		if (fileStreamId == 0) {
			StreamType streamType = ACK_STREAM;
			fileStreamId = xmdTransceiver->createStream(connId, streamType, ACK_STREAM_WAIT_TIME_MS, false);
			callSession.setP2PInternetFileStreamId(fileStreamId);
			XMDLoggerWrapper::instance()->info("file streamId is %d", fileStreamId);
		}
		if (fileStreamId != 0) {
			dataId = xmdTransceiver->sendRTData(connId, fileStreamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}
	}

	delete[] messageBytes;

	XMDLoggerWrapper::instance()->debug("sendRtsDataByP2PInternet, return:%d, uuid:%llu, callId:%llu, connId:%llu, pktType:%d", dataId, user->getUuid(), callId, connId, pktType);

	return dataId;
}

int RtsSendData::sendRtsDataByP2PInternet(User* user, uint64_t callId, const char* data, const unsigned int dataSize, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
    int dataId = -1;

    P2PCallSession& callSession = user->getCurrentCalls()->at(callId);

    if (callSession.isP2PInternetOK() == false)
        return dataId;

    uint64_t connId = callSession.getP2PInternetConnId();

    mimc::UserPacket userPacket;
    userPacket.set_uuid(user->getUuid());
    userPacket.set_from_app_account(user->getAppAccount());
    userPacket.set_resource(user->getResource());
    userPacket.set_pkt_type(pktType);
    userPacket.set_payload(data, dataSize);
    userPacket.set_call_id(callId);
    userPacket.set_region_bucket(user->getRegionBucket());

    int message_size = userPacket.ByteSize();
    char* messageBytes = new char[message_size];
    memset(messageBytes, 0, message_size);
    userPacket.SerializeToArray(messageBytes, message_size);

    uint16_t streamId = 0;

    XMDTransceiver* xmdTransceiver = user->getXmdTransceiver();
    if (pktType == mimc::USER_DATA_AUDIO) {
        streamId = callSession.getP2PInternetAudioStreamId();
        if (streamId == 0) {
            const RtsStreamConfig& audioStreamConfig = user->getStreamConfig(AUDIO);
            StreamType streamType = ACK_STREAM;
            if (audioStreamConfig.getType() == FEC_TYPE) {
                streamType = FEC_STREAM;
            }
            streamId = xmdTransceiver->createStream(connId, streamType, audioStreamConfig.getAckStreamWaitTimeMs(), audioStreamConfig.getEncrypt());
            callSession.setP2PInternetAudioStreamId(streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(connId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }
    } else if (pktType == mimc::USER_DATA_VIDEO) {
        streamId = callSession.getP2PInternetVideoStreamId();
        if (streamId == 0) {
            const RtsStreamConfig& videoStreamConfig = user->getStreamConfig(VIDEO);
            StreamType streamType = FEC_STREAM;
            if (videoStreamConfig.getType() == ACK_TYPE) {
                streamType = ACK_STREAM;
            }
            streamId = xmdTransceiver->createStream(connId, streamType, videoStreamConfig.getAckStreamWaitTimeMs(), videoStreamConfig.getEncrypt());
            callSession.setP2PInternetVideoStreamId(streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(connId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }
    } else if (pktType == mimc::USER_DATA_FILE) {
        streamId = callSession.getP2PInternetFileStreamId();
        if (streamId == 0) {
            StreamType streamType = ACK_STREAM;
            streamId = xmdTransceiver->createStream(connId, streamType, ACK_STREAM_WAIT_TIME_MS, false);
            callSession.setP2PInternetFileStreamId(streamId);
        }
        if (streamId != 0) {
            dataId = xmdTransceiver->sendRTData(connId, streamId, messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
        }
    }

    delete[] messageBytes;

    if((dataId >= 0) && (dataId % 100  < 5))
        XMDLoggerWrapper::instance()->info("sendRtsDataByP2PInternet, dataId:%d, uuid:%llu, callId:%llu, connId:%llu, streamId:%u, pktType:%d", dataId, user->getUuid(), callId, connId, streamId, pktType);

    return dataId;
}

bool RtsSendData::closeRelayConnWhenNoCall(User* user) {
	if (user->getRelayConnId() == 0) {
		return false;
	}
	if (user->currentCallsMapSize() > 0) {
		return false;
	}

	XMDLoggerWrapper::instance()->info("appcount %s has not calls,so close relay connection",user->getAppAccount().c_str());
	user->getXmdTransceiver()->closeConnection(user->getRelayConnId());
	user->resetRelayLinkState();
	
	return true;
}

void* RtsSendData::fetchServerAddr(void* arg) {
    pthread_detach(pthread_self());
    User* user = (User*)arg;
    User::fetchServerAddr(user);
    return NULL;
}
