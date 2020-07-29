#include "mimc/serverfetcher.h"
#include "XMDLoggerWrapper.h"
#include "curl/curl.h"
#include "json-c/json.h"
#include "json-c/bits.h"
#include <map>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif // _WIN32

static json_object* getP2PAddr(json_object* dataobj, const std::string& p2pDomain)
{
	json_object* p2pDomainObj = NULL;
	json_object* p2pAddrArray = NULL;
    int addrArrayLen = 0;

	if(dataobj == NULL || p2pDomain == ""){
		return p2pAddrArray;
	}

    json_object_object_get_ex(dataobj, p2pDomain.c_str(), &p2pDomainObj);
	if (!p2pDomainObj
			|| (json_object_get_type(p2pDomainObj) != json_type_array)
			|| ((addrArrayLen = json_object_array_length(p2pDomainObj)) <= 0)) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr: "
											"received empty or not json array or empty p2pAddr");
		return NULL;	
	}

	p2pAddrArray = json_object_new_array();
	for (int i = 0; i < addrArrayLen; i++) {
		json_object * dataitem = json_object_array_get_idx(p2pDomainObj, i);
		json_object_array_add(p2pAddrArray, json_object_new_string(json_object_get_string(dataitem)));
	}

    return p2pAddrArray;
}

// @param list, concatenate fields feDomain,relayDomain,p2pDomain in order delimited by comma. 
// Notes: @field p2pDomain is optional
std::string ServerFetcher::fetchServerAddr(const char* const &url, std::string list) {

    XMDLoggerWrapper::instance()->debug("ServerFetcher::fetchServerAddr: param list(%s)", list.c_str());

    char delimiter = ',';
	uint32_t pos = list.find(delimiter);
    uint32_t prePos = pos;
	if (pos == std::string::npos) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr param list %s is wrong", list.c_str());
		return "";
	}

	std::string feDomain = list.substr(0, prePos);

	std::string relayDomain = "";
	std::string p2pDomain = "";

	pos = list.find(delimiter, prePos + 1);
    if (pos == std::string::npos) {
        relayDomain = list.substr(prePos + 1);
    } else {
        relayDomain = list.substr(prePos + 1, pos - prePos - 1);
        p2pDomain = list.substr(pos + 1);
    }
    
	std::map<std::string, std::string> paramMap;
	paramMap.insert({"ver", "4.0"});
	paramMap.insert({"type", "wifi"});
	paramMap.insert({"uuid", "0"});
	paramMap.insert({"list", list});
	paramMap.insert({"sdkver", "35"});
	paramMap.insert({"osver", "23"});
	paramMap.insert({"os", "MI%204LTE%3A1.1.1"});
	paramMap.insert({"mi", "2"});

	std::string url_get = url;
	url_get += "?";
	for(std::map<std::string, std::string>::const_iterator iter = paramMap.begin(); iter != paramMap.end(); ++iter) {
		url_get += iter->first + "=" + iter->second + "&";
	}

    CURL *curl = curl_easy_init();
    CURLcode res = CURLE_FAILED_INIT;

    std::string result;
    if (curl) {
       	curl_easy_setopt(curl, CURLOPT_URL, url_get.c_str());
       	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    	struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeRecData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(&result));

        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    if (res != CURLE_OK) {
        XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr curl perform error, error code is %d", res);	
    	return "";
    }

    XMDLoggerWrapper::instance()->debug("ServerFetcher::fetchServerAddr curl perform succeed, result is %s\n", result.c_str());

    json_object * pobj = json_tokener_parse(result.c_str());
	if(is_error(pobj)) {
		XMDLoggerWrapper::instance()->error("json_tokener_parse failed");
		return "";
	}
    json_object * dataobj = NULL;
    const char* dataItem = NULL;
	json_object_object_get_ex(pobj, "S", &dataobj);
	if(NULL == dataobj) {
		XMDLoggerWrapper::instance()->error("json get S failed");
		return "";
	}
	dataItem = json_object_get_string(dataobj);
	if(NULL == dataItem) {
		XMDLoggerWrapper::instance()->error("json get dataItem failed");
		return "";
	}
	if (strcasecmp(dataItem, "OK") != 0) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr receive data error, status is %s", dataItem);
		return "";
	}
	json_object * datafeobj = NULL;
	json_object * datarelayobj = NULL;
	json_object_object_get_ex(pobj, "R", &dataobj);
	json_object_object_get_ex(dataobj, "wifi", &dataobj);
	json_object_object_get_ex(dataobj, feDomain.c_str(), &datafeobj);
	if (!datafeobj) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr receive data error, feAddr is null");
		return "";	
	}
	if (json_object_get_type(datafeobj) != json_type_array) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr receive data error, feAddr is not json array");
		return "";
	}
	int array_size = 0;
	array_size = json_object_array_length(datafeobj);
	if (array_size == 0) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr receive data error, feAddr is empty array");
		return "";
	}
	json_object * resdatafeobj = json_object_new_array();
	for (int i = 0; i < array_size; i++) {
		json_object * dataitem = json_object_array_get_idx(datafeobj, i);
		json_object_array_add(resdatafeobj, json_object_new_string(json_object_get_string(dataitem)));
	}
	json_object * resobj = json_object_new_object();
	json_object_object_add(resobj, feDomain.c_str(), resdatafeobj);
	json_object_object_get_ex(dataobj, relayDomain.c_str(), &datarelayobj);
	if (!datarelayobj) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr receive data error, relayAddr is empty");
		return "";	
	}
	if (json_object_get_type(datarelayobj) != json_type_array) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr receive data error, relayAddr is not json array");
		return "";
	}
	array_size = json_object_array_length(datarelayobj);
	if (array_size == 0) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr receive data error, relayAddr is empty array");
		return "";
	}
	json_object * resdatarelayobj = json_object_new_array();
	for (int i = 0; i < array_size; i++) {
		json_object * dataitem = json_object_array_get_idx(datarelayobj, i);
		json_object_array_add(resdatarelayobj, json_object_new_string(json_object_get_string(dataitem)));
	}
	json_object_object_add(resobj, relayDomain.c_str(), resdatarelayobj);

	json_object* p2pAddrArray = NULL;
    if (p2pDomain != "" 
            && ((p2pAddrArray = getP2PAddr(dataobj, p2pDomain)) != NULL)) {
        json_object_object_add(resobj, p2pDomain.c_str(), p2pAddrArray);
    }

	result.assign(json_object_get_string(resobj));
	json_object_put(resobj);
	json_object_put(pobj);
    return result;
}

size_t ServerFetcher::writeRecData(char* ptr, size_t size, size_t nmemb, void* userdata) {
	std::string* body = (std::string *)userdata;
	body->append(ptr, size*nmemb);
	return size*nmemb;
}
