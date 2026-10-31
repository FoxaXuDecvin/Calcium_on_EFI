
//Third Party Code
//Here
#include"../shload.h"

bool creatpath(std::string fileaddress) {


	//For Windows System
	if (_Run_SysKernel == Win32_kernel) {
		if (!checkChar(fileaddress, ":")) {
			fileaddress = _$GetSelfPath + "/" + fileaddress;
		}
	}

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

	if (checkChar(outdata, "/")) creatpath(outdata);
	_dapi_mkdir(outdata);
	return true;

}