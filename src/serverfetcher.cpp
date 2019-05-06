#include <mimc/serverfetcher.h>
#include <XMDLoggerWrapper.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <map>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif // _WIN32

std::string ServerFetcher::fetchServerAddr(const char* const &url, std::string list) {
	int pos = list.find(",");
	if (pos == std::string::npos) {
		XMDLoggerWrapper::instance()->error("ServerFetcher::fetchServerAddr param list %s is wrong", list.c_str());
		return "";
	}
	std::string feDomain = list.substr(0, pos);
	std::string relayDomain = list.substr(pos+1);

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
    CURLcode res;

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
    json_object * dataobj = NULL;
    const char* dataItem = NULL;
	json_object_object_get_ex(pobj, "S", &dataobj);
	dataItem = json_object_get_string(dataobj);
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