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

	string getData() {return this->data;}
	long getDataTime() {return this->dataTime;}
	long getLoginTime1() {return this->loginTime1;}
	long getLoginTime2() {return this->loginTime2;}
	long getDialCallTime() {return this->dialCallTime;}
	long getRecvDataTime() {return this->recvDataTime;}

private:
	string data;
	long dataTime;
	long loginTime1;
	long loginTime2;
	long dialCallTime;
	long recvDataTime;
};

#endif