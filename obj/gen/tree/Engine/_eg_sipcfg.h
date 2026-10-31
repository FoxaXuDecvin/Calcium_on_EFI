//Base Engine
//Simple File config Engine
//Copyright FoxaXu 2024

#include"../shload.h"
#define _CRT_SECURE_NO_WARNINGS
std::string tempreadbuffEr_;

//Design On Calcium Project
std::string tempptr, readcacheA;
int readptr_glo = 0;

std::string _load_sipcfg(std::string _sc_File, std::string _sc_ID) {
	using namespace std;
	int readptr = 0;
	if (check_file_existenceA(_sc_File)) {
		while (true) {
			readptr++;
			tempptr = _fileapi_textread(_sc_File, readptr);
			if (tempptr == "") {
				continue;
			}
			if (tempptr == "ReadFailed") {
				if (readptr != 1) {
					continue;
				}
				cout << "Failed to Load Simple Config" << endl;
				cout << "Open File Failed :  " << _sc_File << endl;
				break;
			}
			if (tempptr == "overline") {
				_sh_throw_error("Load Simple Config Error :  Unknown Config :  Not Found  --(" + _sc_ID + ")--  On File :    _" + _sc_File + "_");
				return "";
			}
			readcacheA = _api_PartRead(tempptr, "$", "=");

			//cout << "Read Up :  Line " << readptr << "   INFO :  _" << tempptr << "_" << endl;

			if (readcacheA == _sc_ID) {
				return _api_PartRead(tempptr, "=", ";");
			}

			if (tempptr == "")break;

			continue;
		}
		return "";
	}
	else {
		_sh_throw_error("Load Simple Config Error :  File Not Found   _" + _sc_File + "_");
		return "";
	}
}

std::string _load_sipcfg_noreturn(std::string _sc_File, std::string _sc_ID) {
	using namespace std;
	if (check_file_existenceA(_sc_File)) {
		while (true) {
			readptr_glo++;
			tempptr = _fileapi_textread(_sc_File, readptr_glo);
			if (tempptr == "") {
				continue;
			}
			if (tempptr == "ReadFailed") {
				if (readptr_glo != 1) {
					continue;
				}
				cout << "Failed to Load Simple Config" << endl;
				cout << "Open File Failed :  " << _sc_File << endl;
				break;
			}
			if (tempptr == "overline") {
				_sh_throw_error("Load Simple Config Error :  Unknown Config :  Not Found  --(" + _sc_ID + ")--  On File :    _" + _sc_File + "_");
				return "";
			}
			readcacheA = _api_PartRead(tempptr, "$", "=");

			//cout << "Read Up :  Line " << readptr << "   INFO :  _" << tempptr << "_" << endl;

			if (readcacheA == _sc_ID) {
				return _api_PartRead(tempptr, "=", ";");
			}

			if (tempptr == "")break;

			continue;
		}
		return "";
	}
	else {
		_sh_throw_error("Load Simple Config Error :  File Not Found   _" + _sc_File + "_");
		return "";
	}
}

void _sipcfg_reset(void) {
	readptr_glo = 0;
	return;
}

bool _sw_target_remove;
bool _spcfg_wiriteapi(std::string _sc_File, int _sc_Line, std::string _sc_header, std::string _sc_winfo) {
	//Simple Config Write
	_sw_target_remove = false;
	//Create New File

	std::string _scnew_file = "sipcfg_newtemp-" + _get_random_s(1, 99999);


	int n_readptr_ = 1;
	while (true) {
		//File Write
		if (n_readptr_ == _sc_Line) {
			//ReCreate Config
			if (_sc_winfo != "@target_sipcfg_remove") {
				_fileapi_write(_scnew_file, "$" + _sc_header + "=" + _sc_winfo + ";");
				_sw_target_remove = false;
			}
			else {
				_sw_target_remove = true;
			}
			n_readptr_++;
			continue;
		}

		ReTryReader:
		tempreadbuffEr_ = _fileapi_textread(_sc_File, n_readptr_);

		if (tempreadbuffEr_ == "overline") {
			break;
		}

		if (tempreadbuffEr_ == "ReadFailed") {
			_p("fileapi.read error.  address " + _sc_File + ".  level " + std::to_string(n_readptr_) + "   Retry !");
			goto ReTryReader;
		}

		if (tempreadbuffEr_ == "LineError") {
			break;
		}

		_fileapi_write(_scnew_file, tempreadbuffEr_);

		n_readptr_++;
		continue;
	}

	remove(_sc_File.c_str());
	CopyFile(_scnew_file.c_str(), _sc_File.c_str(), 0);
	remove(_scnew_file.c_str());

	if (_sw_target_remove == false) {
		if (_load_sipcfg(_sc_File, _sc_header) == _sc_winfo) {
			return true;
		}
		else {
			_sh_throw_error("Process Config Failed...");
			return false;
		}
	}
	else {
		_sw_target_remove = false;
	}

	return false;
}

bool _write_sipcfg(std::string _sc_File, std::string _sc_ID, std::string _sc_writeINFO) {
	using namespace std;
	std::string readbufferA;
	int readptr = 0;
	if (check_file_existenceA(_sc_File)) {
		while (true) {
			readptr++;
			std::string tempptr = _fileapi_textread(_sc_File, readptr);
			if (tempptr == "") {
				continue;
			}
			if (tempptr == "ReadFailed") {
				if (readptr == 1) {
					continue;
				}
				cout << "Failed to Write Simple Config" << endl;
				cout << "Open File Failed :  " << _sc_File << endl;
				break;
			}

			if (tempptr == "overline") {
				//WriteNewConfig
				return _spcfg_wiriteapi(_sc_File, readptr, _sc_ID, _sc_writeINFO);
			}
			readbufferA = _api_PartRead(tempptr, "$", "=");

			//cout << "Read Up :  Line " << readptr << "   INFO :  _" << tempptr << "_" << endl;

			if (readbufferA == _sc_ID) {
				return _spcfg_wiriteapi(_sc_File, readptr, _sc_ID, _sc_writeINFO);
			}

			if (tempptr == "")break;

			continue;
		}
		return "";
	}
	else {
		_sh_throw_error("Write Simple Config Error :  File Not Found   _" + _sc_File + "_");
		return "";
	}
}

//New Remove API
std::string rs_buf;
bool _remove_sipcfg(std::string _sc_File, std::string _sc_ID) {
	using namespace std;
	std::string readbufferA;
	int readptr = 0;
	if (check_file_existenceA(_sc_File)) {
		while (true) {
			readptr++;
			std::string tempptr = _fileapi_textread(_sc_File, readptr);
			if (tempptr == "") {
				continue;
			}
			if (tempptr == "ReadFailed") {
				cout << "Failed to Write Simple Config" << endl;
				cout << "Open File Failed :  " << _sc_File << endl;
				break;
			}
			if (tempptr == "overline") {
				//WriteNewConfig
				return _spcfg_wiriteapi(_sc_File, readptr, _sc_ID, "@target_sipcfg_remove");
			}
			readbufferA = _api_PartRead(tempptr, "$", "=");

			//cout << "Read Up :  Line " << readptr << "   INFO :  _" << tempptr << "_" << endl;

			if (readbufferA == _sc_ID) {
				return _spcfg_wiriteapi(_sc_File, readptr, _sc_ID, "@target_sipcfg_remove");
			}

			if (tempptr == "")break;

			continue;
		}
		return "";
	}
	else {
		_sh_throw_error("Write Simple Config Error :  File Not Found   _" + _sc_File + "_");
		return "";
	}
}
