#include <mimc/rts_send_data.h>
#include <mimc/user.h>
#include <mimc/rts_connection_info.h>
#include <mimc/utils.h>
#include <XMDTransceiver.h>
#include <cstdlib>

uint64_t RtsSendData::createRelayConn(User* user) {
if (user->getRelayConnId() != 0) {
	return user->getRelayConnId();
}
std::string relayAddress;
#ifndef STAGING
	if (!User::fetchServerAddr(user)) {
		return 0;
	}
	srand(Utils::currentTimeMicros());

	pthread_mutex_lock(&user->getAddressMutex());
	std::vector<std::string>& relayAddresses = user->getRelayAddresses();
	if (!relayAddresses.empty()) {
		relayAddress = relayAddresses.at(rand()%(relayAddresses.size()));
	}
	pthread_mutex_unlock(&user->getAddressMutex());
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
	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(mimc::RELAY_CONN_REQUEST);

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	XMDLoggerWrapper::instance()->info("In createRelayConn, relayIp is %s", relayIp.c_str());

	uint64_t relayConnId = user->getXmdTransceiver()->createConnection((char *)relayIp.c_str(), relayPort, messageBytes, message_size, XMD_TRAN_TIMEOUT, new RtsConnectionInfo(relayAddress, RELAY_CONN));
	if (relayConnId == 0) {
		return 0;
	}

	user->setRelayConnId(relayConnId);
	user->setRelayLinkState(BEING_CREATED);
	user->setLatestLegalRelayLinkStateTs(time(NULL));
	delete[] messageBytes;
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

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	if (user->getXmdTransceiver()->sendRTData(user->getRelayConnId(), user->getRelayControlStreamId(), messageBytes, message_size) < 0) {
		XMDLoggerWrapper::instance()->warn("In sendBindRelayRequest, sendRTData failed");
		return false;
	}

	XMDLoggerWrapper::instance()->info("In sendBindRelayRequest, sendRTData succeed");
	delete[] userPacketPayload;
	delete[] messageBytes;
	return true;
}

bool RtsSendData::sendPingRelayRequest(User* user) {
	pthread_rwlock_rdlock(&user->getCallsRwlock());
	if (user->getCurrentCalls()->empty() || user->getRelayLinkState() != SUCC_CREATED) {
		pthread_rwlock_unlock(&user->getCallsRwlock());
		return false;
	}

	pthread_rwlock_unlock(&user->getCallsRwlock());
	mimc::PingRelayRequest pingRelayRequest;
	pingRelayRequest.set_uuid(user->getUuid());
	pingRelayRequest.set_resource(user->getResource());

	int userPacketPayloadSize = pingRelayRequest.ByteSize();
	char* userPacketPayload = new char[userPacketPayloadSize];
	memset(userPacketPayload, 0, userPacketPayloadSize);
	pingRelayRequest.SerializeToArray(userPacketPayload, userPacketPayloadSize);

	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_resource(user->getResource());
	userPacket.set_payload(userPacketPayload, userPacketPayloadSize);
	userPacket.set_pkt_type(mimc::PING_RELAY_REQUEST);

	int message_size = userPacket.ByteSize();
	char* messageBytes = new char[message_size];
	memset(messageBytes, 0, message_size);
	userPacket.SerializeToArray(messageBytes, message_size);

	if (user->getXmdTransceiver()->sendRTData(user->getRelayConnId(), user->getRelayControlStreamId(), messageBytes, message_size, true, P1, 0, NULL) < 0) {
		
		return false;
	}
	
	delete[] userPacketPayload;
	delete[] messageBytes;
	return true;
}

int RtsSendData::sendRtsDataByRelay(User* user, uint64_t callId, const std::string& data, const mimc::PKT_TYPE pktType, const void* ctx, const bool canBeDropped, const DataPriority priority, const unsigned int resendCount) {
	int dataId = -1;
	uint64_t relayConnId = user->getRelayConnId();
	if (relayConnId == 0) {
		
		return dataId;
	}
	mimc::UserPacket userPacket;
	userPacket.set_uuid(user->getUuid());
	userPacket.set_from_app_account(user->getAppAccount());
	userPacket.set_resource(user->getResource());
	userPacket.set_pkt_type(pktType);
	userPacket.set_payload(std::string(data));
	userPacket.set_call_id(callId);

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
		}
	} else if (pktType == mimc::USER_DATA_VIDEO) {
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
		}
	} else if (pktType == mimc::USER_DATA_FILE) {
		if (user->getRelayFileStreamId() == 0) {
			user->setRelayFileStreamId(xmdTransceiver->createStream(relayConnId, ACK_STREAM, ACK_STREAM_WAIT_TIME_MS, false));
			XMDLoggerWrapper::instance()->info("file streamId is %d", user->getRelayFileStreamId());
		}
		if (user->getRelayFileStreamId() != 0) {
			dataId = xmdTransceiver->sendRTData(relayConnId, user->getRelayFileStreamId(), messageBytes, message_size, canBeDropped, priority, resendCount, (void *)ctx);
		}
	}

	delete[] messageBytes;
	return dataId;
}

bool RtsSendData::closeRelayConnWhenNoCall(User* user) {
	if (user->getRelayConnId() == 0) {
		
		return false;
	}
	if (user->getCurrentCalls()->size() > 0) {
		
		return false;
	}
	user->getXmdTransceiver()->closeConnection(user->getRelayConnId());
	user->resetRelayLinkState();
	
	return true;
}