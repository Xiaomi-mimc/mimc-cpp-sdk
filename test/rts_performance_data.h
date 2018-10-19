#ifndef MIMC_CPP_TEST_RTSPERFORMANCEDATA_H
#define MIMC_CPP_TEST_RTSPERFORMANCEDATA_H

#include <string>

using namespace std;

class RtsPerformanceData
{
public:
	RtsPerformanceData(){}
	
	RtsPerformanceData(const string& data, long dataTime) {
		this->data = data;
		this->dataTime = dataTime;
	}

	RtsPerformanceData(long loginTime1, long loginTime2, long dialCallTime, long recvDataTime) {
		this->loginTime1 = loginTime1;
		this->loginTime2 = loginTime2;
		this->dialCallTime = dialCallTime;
		this->recvDataTime = recvDataTime;
	}

	const string& getData() const {return this->data;}
	long getDataTime() const {return this->dataTime;}
	long getLoginTime1() const {return this->loginTime1;}
	long getLoginTime2() const {return this->loginTime2;}
	long getDialCallTime() const {return this->dialCallTime;}
	long getRecvDataTime() const {return this->recvDataTime;}

private:
	string data;
	long dataTime;
	long loginTime1;
	long loginTime2;
	long dialCallTime;
	long recvDataTime;
};

#endif