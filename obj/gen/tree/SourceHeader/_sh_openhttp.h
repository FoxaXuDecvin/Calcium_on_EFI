// Http/Https Download Toolkit
// Base System API
// Copyright FoxaXu 2024

#include "../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS

bool _$BufferCache;

//URLDown

//return True/False  =   result
bool _urldown_api(std::string url, std::string savepath) {
	//Disabled Cache
	_$BufferCache = URLDown(url, savepath);

	//Decide
	if (check_file_existenceA(savepath)) {
		return true;
	}
	else {
		return false;
	}

	//No Code
}

bool _urldown_api_nocache(std::string url, std::string savepath) {
	url = url + "?voidcache=" + _get_random_s(1, 10000);
	//Disabled Cache
	_$BufferCache = URLDown(url, savepath);

	//Decide
	if (check_file_existenceA(savepath)) {
		return true;
	}
	else {
		return false;
	}

	//No Code
}

bool _urldown_api_vc_nocache(std::string url, std::string savepathA) {
	std::string savepath = _get_random_s(1, 11111111);
	url = url + "?voidcache=" + _get_random_s(1, 10000);
	//Disabled Cache
	_$BufferCache = URLDown(url, savepath);

	//Decide
	if (check_file_existenceA(savepath)) {
		rename(savepath.c_str(), savepathA.c_str());
		return true;
	}
	else {
		_sh_throw_error("OpenHttp.Error :  Failed to Get URL :   " + url);
		return false;
	}

	//No Code
}

//END