#ifndef MIMC_CPP_SDK_TOKENFETCHER_H
#define MIMC_CPP_SDK_TOKENFETCHER_H

#include <string>

#ifdef WIN_USE_DLL
#ifdef MIMCAPI_EXPORTS
#define MIMCAPI __declspec(dllexport)
#else
#define MIMCAPI __declspec(dllimport)
#endif // MIMCAPI_EXPORTS
#else
#define MIMCAPI
#endif

#ifdef _WIN32
class MIMCAPI MIMCTokenFetcher {
#else
class MIMCTokenFetcher {
#endif // _WIN32

	public:
		virtual std::string fetchToken() = 0;
		virtual ~MIMCTokenFetcher() {}
};

#endif