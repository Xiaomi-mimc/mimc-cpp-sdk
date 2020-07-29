#ifndef MIMC_CPP_SDK_RTS_CALLEVENTHANDLER_H
#define MIMC_CPP_SDK_RTS_CALLEVENTHANDLER_H

#include <string>
#include <mimc/launchedresponse.h>
#include <mimc/constant.h>

class RTSCallEventHandler {
public:
	/**
	 * 新会话接入回调
	 **/
	virtual LaunchedResponse onLaunched(uint64_t callId, const std::string& fromAccount, const std::string& appContent, const std::string& fromResource) = 0;
	/**
	 * 会话被接通回调
	 **/
	virtual void onAnswered(uint64_t callId, bool accepted, const std::string& desc) = 0;

	/**
	 * 会话被关闭回调
	 **/
	virtual void onClosed(uint64_t callId, const std::string& desc) = 0;

	/**
	 * 接收到数据的回调
	 **/
	virtual void onData(uint64_t callId, const std::string& fromAccount, const std::string& resource, const std::string& data, RtsDataType dataType, RtsChannelType channelType) = 0;

	/**
     * 发送数据成功的回调
     */
	virtual void onSendDataSuccess(uint64_t callId, int dataId, const std::string& ctx) = 0;

	/**
     * 发送数据失败的回调
     */
	virtual void onSendDataFailure(uint64_t callId, int dataId, const std::string& ctx) = 0;


	/**
     * P2P尝试结果回调,added by huanghua for p2p
     */
	virtual void onP2PResult(uint64_t callId, int result, int selfNatType, int peerNatType) {}

	virtual ~RTSCallEventHandler() {}
};
#endif