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

	RtsPerformanceData(int loginTime1, int loginTime2, int loginTime3, int dialCallTime1, int dialCallTime2, int dialCallTime3, int recvDataTime1, int recvDataTime2, int recvDataTime3) {
		this->loginTime1 = loginTime1;
		this->loginTime2 = loginTime2;
		this->loginTime3 = loginTime3;
		this->dialCallTime1 = dialCallTime1;
		this->dialCallTime2 = dialCallTime2;
		this->dialCallTime3 = dialCallTime3;
		this->recvDataTime1 = recvDataTime1;
		this->recvDataTime2 = recvDataTime2;
		this->recvDataTime3 = recvDataTime3;
	}

	const string& getData() const {return this->data;}
	long getDataTime() const {return this->dataTime;}
	int getLoginTime1() const {return this->loginTime1;}
	int getLoginTime2() const {return this->loginTime2;}
	int getLoginTime3() const {return this->loginTime3;}
	int getDialCallTime1() const {return this->dialCallTime1;}
	int getDialCallTime2() const {return this->dialCallTime2;}
	int getDialCallTime3() const {return this->dialCallTime3;}
	int getRecvDataTime1() const {return this->recvDataTime1;}
	int getRecvDataTime2() const {return this->recvDataTime2;}
	int getRecvDataTime3() const {return this->recvDataTime3;}

private:
	string data;
	long dataTime;
	int loginTime1;
	int loginTime2;
	int loginTime3;
	int dialCallTime1;
	int dialCallTime2;
	int dialCallTime3;
	int recvDataTime1;
	int recvDataTime2;
	int recvDataTime3;
};

#endif