#ifndef MIMC_CPP_SDK_P2PCALLSESSION_H
#define MIMC_CPP_SDK_P2PCALLSESSION_H

#include <mimc/rts_signal.pb.h>
#include <string>
#include <stdint.h>

enum CallState {
	WAIT_SEND_CREATE_REQUEST,
	WAIT_CALL_ONLAUNCHED,
	WAIT_INVITEE_RESPONSE,
	WAIT_CREATE_RESPONSE,
	RUNNING,
	WAIT_SEND_UPDATE_REQUEST,
	WAIT_UPDATE_RESPONSE
};

class P2PCallSession {
public:
	P2PCallSession(uint64_t callId, mimc::UserInfo peerUser, mimc::CallType callType, CallState callState, time_t callStateTs, bool is_creator, std::string appContent) 
		: callId(callId), peerUser(peerUser), callType(callType), callState(callState), latestLegalCallStateTs(callStateTs), is_creator(is_creator), appContent(appContent)
	{
		clearP2PConn();
	}

	mimc::UserInfo getPeerUser() const{ return this->peerUser; }
	void setPeerUser(const mimc::UserInfo& peerUser) { this->peerUser = peerUser; }

	mimc::CallType getCallType() const{ return this->callType; }
	void setCallType(const mimc::CallType& callType) { this->callType = callType; }

	CallState getCallState() const{ return this->callState; }
	void setCallState(const CallState& callState) { this->callState = callState; }

	std::string getAppContent() const{ return this->appContent; }

	time_t getLatestLegalCallStateTs() const{ return this->latestLegalCallStateTs; }
	void setLatestLegalCallStateTs(const time_t& latestLegalCallStateTs) { this->latestLegalCallStateTs = latestLegalCallStateTs; }

	uint64_t getP2PIntranetConnId() const{ return this->P2PIntranetConnId; }
	void setP2PIntranetConnId(const uint64_t& P2PIntranetConnId) { this->P2PIntranetConnId = P2PIntranetConnId; }

	uint16_t getP2PIntranetVideoStreamId() const{ return this->P2PIntranetVideoStreamId; }
	void setP2PIntranetVideoStreamId(const uint16_t& P2PIntranetVideoStreamId) { this->P2PIntranetVideoStreamId = P2PIntranetVideoStreamId; }

	uint16_t getP2PIntranetAudioStreamId() const{ return this->P2PIntranetAudioStreamId; }
	void setP2PIntranetAudioStreamId(const uint16_t& P2PIntranetAudioStreamId) { this->P2PIntranetAudioStreamId = P2PIntranetAudioStreamId; }

	bool getIntranetBurrowState() const{ return this->intranetBurrowState; }
	void setIntranetBurrowState(const bool& intranetBurrowState) { this->intranetBurrowState = intranetBurrowState; }

	uint64_t getP2PInternetConnId() const{ return this->P2PInternetConnId; }
	void setP2PInternetConnId(const uint64_t& P2PInternetConnId) { this->P2PInternetConnId = P2PInternetConnId; }

	uint16_t getP2PInternetVideoStreamId() const{ return this->P2PInternetVideoStreamId; }
	void setP2PInternetVideoStreamId(const uint16_t& P2PInternetVideoStreamId) { this->P2PInternetVideoStreamId = P2PInternetVideoStreamId; }

	uint16_t getP2PInternetAudioStreamId() const{ return this->P2PInternetAudioStreamId; }
	void setP2PInternetAudioStreamId(const uint16_t& P2PInternetAudioStreamId) { this->P2PInternetAudioStreamId = P2PInternetAudioStreamId; }

	bool getInternetBurrowState() const{ return this->internetBurrowState; }
	void setInternetBurrowState(const bool& internetBurrowState) { this->internetBurrowState = internetBurrowState; }

	bool isCreator() const{ return this->is_creator; }

	void resetP2PIntranetConn() { 
		setIntranetBurrowState(false);
		setP2PIntranetConnId(0);
		setP2PIntranetAudioStreamId(0);
		setP2PIntranetVideoStreamId(0);
	}

	void resetP2PInternetConn() { 
		setInternetBurrowState(false);
		setP2PInternetConnId(0);
		setP2PInternetAudioStreamId(0);
		setP2PInternetVideoStreamId(0);
	}

	void clearP2PConn() {
		resetP2PIntranetConn();
		resetP2PInternetConn();
	}
private:
	uint64_t callId;
	mimc::UserInfo peerUser;
	mimc::CallType callType;
	CallState callState;
	time_t latestLegalCallStateTs;

	uint64_t P2PIntranetConnId;
	uint16_t P2PIntranetVideoStreamId;
	uint16_t P2PIntranetAudioStreamId;
	bool intranetBurrowState;

	uint64_t P2PInternetConnId;
	uint16_t P2PInternetVideoStreamId;
	uint16_t P2PInternetAudioStreamId;
	bool internetBurrowState;

	bool is_creator;

	std::string appContent;
};

#endif