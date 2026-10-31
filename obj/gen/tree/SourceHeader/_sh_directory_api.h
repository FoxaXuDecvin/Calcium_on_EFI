//Directory API
// Copyright FoxaXu 2024

#include "../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS
//DEF
bool _fileapi_createmark(std::string File, std::string info);
int _get_random(int min, int max);


//MAIN
bool _dapi_ExistFolder_check(std::string dir) {
	using namespace std;
	std::string tmpDoct = dir + pathsign + "test~" + std::to_string(_get_random(1, 999999)) + "~.mark";

	if (_fileapi_createmark(tmpDoct, "nWn")) {
		remove(tmpDoct.c_str());
		return true;
	}
	else {
		remove(tmpDoct.c_str());
		return false;
	}
}

bool _dapi_rmdir(std::string dir) {
	using namespace std;
	dir = ReplaceChar(dir, "\\", "/");

	removeDirectoryAPIX(dir.c_str());

	//cout << "Remove Directory :  " << dir << endl;

	if (_dapi_ExistFolder_check(dir)) {
		return false;
	}
	else {
		return true;
	}
}

bool _dapi_mkdir(std::string dir) {
	using namespace std;
	if (_Run_SysKernel == Win32_kernel) {
		dir = ReplaceChar(dir, "/", "\\");
	}

	if (_Run_SysKernel == Linux_kernel) {
		dir = ReplaceChar(dir, "\\", "/");
	}

	foldercreateapi(dir.c_str());

	//cout << "Remove Directory :  " << dir << endl;

	if (_dapi_ExistFolder_check(dir)) {
		return true;
	}
	else {
		return false;
	}
}

bool cp_true = false;
bool _dapi_create_full_path(std::string fileaddress) {
	using namespace std;
	std::string tempdata, outdata;

	if (checkChar(fileaddress, "/")) cp_true = true;
	if (checkChar(fileaddress, "\\")) cp_true = true;

	if (cp_true == false) {
		return false;
	}

	fileaddress = ReplaceChar(fileaddress, "\\", "/");

	int numbuffer = fileaddress.size();
	int baseNum = -1;

	while (true) {
		tempdata = fileaddress[numbuffer];
		if (tempdata == "/") break;
		numbuffer--;
	}
	numbuffer--;
	while (baseNum != numbuffer) {
		baseNum++;
		outdata = outdata + fileaddress[baseNum];
	};

	if (checkChar(outdata, "/")) _dapi_create_full_path(outdata);
	_dapi_mkdir(outdata);
	return true;

}