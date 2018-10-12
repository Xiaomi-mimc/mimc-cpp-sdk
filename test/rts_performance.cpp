#include <gtest/gtest.h>
#include <mimc/user.h>
#include <mimc/utils.h>
#include <mimc/error.h>

using namespace std;

string appId = "2882303761517613988";
string appKey = "5361761377988";
string appSecret = "2SZbrJOAL1xHRKb7L9AiRQ==";
string appAccount1 = "perf_MI153";
string appAccount2 = "perf_MI108";

class RtsPerformance: public testing::Test {
protected:

protected:
	User* rtsUser1;
	User* rtsUser2;
};