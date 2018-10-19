#ifndef MIMC_CPP_SDK_P2PCHATSESSION_H
#define MIMC_CPP_SDK_P2PCHATSESSION_H

#include <mimc/rts_signal.pb.h>
#include <string>

enum ChatState {
	WAIT_SEND_CREATE_REQUEST,
	WAIT_CALL_ONLAUNCHED,
	WAIT_INVITEE_RESPONSE,
	WAIT_CREATE_RESPONSE,
	RUNNING,
	WAIT_SEND_UPDATE_REQUEST,
	WAIT_UPDATE_RESPONSE
};

class P2PChatSession {
public:
	P2PChatSession(long chatId, mimc::UserInfo peerUser, mimc::ChatType chatType, ChatState chatState, long chatStateTs, bool is_creator, std::string appContent) 
		: chatId(chatId), peerUser(peerUser), chatType(chatType), chatState(chatState), latestLegalChatStateTs(chatStateTs), is_creator(is_creator), appContent(appContent)
	{
		clearP2PConn();
	}

	mimc::UserInfo getPeerUser() const{ return this->peerUser; }
	void setPeerUser(const mimc::UserInfo& peerUser) { this->peerUser = peerUser; }

	mimc::ChatType getChatType() const{ return this->chatType; }
	void setChatType(const mimc::ChatType& chatType) { this->chatType = chatType; }

	ChatState getChatState() const{ return this->chatState; }
	void setChatState(const ChatState& chatState) { this->chatState = chatState; }

	std::string getAppContent() const{ return this->appContent; }

	long getLatestLegalChatStateTs() const{ return this->latestLegalChatStateTs; }
	void setLatestLegalChatStateTs(const long& latestLegalChatStateTs) { this->latestLegalChatStateTs = latestLegalChatStateTs; }

	long getP2PIntranetConnId() const{ return this->P2PIntranetConnId; }
	void setP2PIntranetConnId(const long& P2PIntranetConnId) { this->P2PIntranetConnId = P2PIntranetConnId; }

	short getP2PIntranetVideoStreamId() const{ return this->P2PIntranetVideoStreamId; }
	void setP2PIntranetVideoStreamId(const short& P2PIntranetVideoStreamId) { this->P2PIntranetVideoStreamId = P2PIntranetVideoStreamId; }

	short getP2PIntranetAudioStreamId() const{ return this->P2PIntranetAudioStreamId; }
	void setP2PIntranetAudioStreamId(const short& P2PIntranetAudioStreamId) { this->P2PIntranetAudioStreamId = P2PIntranetAudioStreamId; }

	bool getIntranetBurrowState() const{ return this->intranetBurrowState; }
	void setIntranetBurrowState(const bool& intranetBurrowState) { this->intranetBurrowState = intranetBurrowState; }

	long getP2PInternetConnId() const{ return this->P2PInternetConnId; }
	void setP2PInternetConnId(const long& P2PInternetConnId) { this->P2PInternetConnId = P2PInternetConnId; }

	short getP2PInternetVideoStreamId() const{ return this->P2PInternetVideoStreamId; }
	void setP2PInternetVideoStreamId(const short& P2PInternetVideoStreamId) { this->P2PInternetVideoStreamId = P2PInternetVideoStreamId; }

	short getP2PInternetAudioStreamId() const{ return this->P2PInternetAudioStreamId; }
	void setP2PInternetAudioStreamId(const short& P2PInternetAudioStreamId) { this->P2PInternetAudioStreamId = P2PInternetAudioStreamId; }

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
	long chatId;
	mimc::UserInfo peerUser;
	mimc::ChatType chatType;
	ChatState chatState;
	long latestLegalChatStateTs;

	long P2PIntranetConnId;
	short P2PIntranetVideoStreamId;
	short P2PIntranetAudioStreamId;
	bool intranetBurrowState;

	long P2PInternetConnId;
	short P2PInternetVideoStreamId;
	short P2PInternetAudioStreamId;
	bool internetBurrowState;

	bool is_creator;

	std::string appContent;
};

#endif