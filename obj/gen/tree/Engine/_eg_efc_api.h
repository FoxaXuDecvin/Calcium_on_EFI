//from Shield (3013) After
// EFC API will not include in shload.h

//Easy File Connect API

#include"../shload.h"
#define _CRT_SECURE_NO_WARNINGS
//Copyright FoxaXu 2024
//File Connect API
// Use File monitor
//Based on SipCfg(_eg_sipcfg.h)

std::string efc_ver = "r10.1.1.1";

//initialize efc
bool _efc_prep(std::string Create_EFC_File,std::string socketid) {
	efc_Monitor = Create_EFC_File;

	if (check_file_existenceA(efc_Monitor)) {
		remove(efc_Monitor.c_str());
		if (check_file_existenceA(efc_Monitor)) {
			_sh_throw_error("efcapi_error_create.connect.monitor.failed");
			return false;
		}
	}

	_fileapi_createmark(efc_Monitor, "Protocol // File Connect API :  EFC  __ " + socketid);

	_write_sipcfg(efc_Monitor, "ProtocolVersion", efc_ver);
	_write_sipcfg(efc_Monitor, "SocketID", socketid);
	
	if (!check_file_existenceA(efc_Monitor)) {
		_sh_throw_error("efcapi_error_create.connect.monitor.access.denied");
		return false;
	}

	return true;
}

bool _efc_create_port(std::string portid) {
	if (!check_file_existenceA(efc_Monitor)) {
		_sh_throw_error("efcapi_error_port.not.initialize");
		return false;
	}

	if (_load_sipcfg(efc_Monitor, "ProtocolVersion") != efc_ver) {
		if (_efc_settings$void_protocol_version_check == false) {
			_sh_throw_error("efcapi_warning_protocol.not.the.same");
		}
	}

	//Create New Port
	_write_sipcfg(efc_Monitor, portid, "ready");

	return true;
}

//Status ID
//Default :   ready
bool _efc_setstatus_port(std::string portid, std::string status) {
	if (!check_file_existenceA(efc_Monitor)) {
		_sh_throw_error("efcapi_error_port.not.initialize");
		return false;
	}

	if (_load_sipcfg(efc_Monitor, "ProtocolVersion") != efc_ver) {
		if (_efc_settings$void_protocol_version_check == false) {
			_sh_throw_error("efcapi_warning_protocol.not.the.same");
		}
	}

	_write_sipcfg(efc_Monitor, portid, status);
	return true;
}

std::string _efc_getstatus_port(std::string portid) {
	if (!check_file_existenceA(efc_Monitor)) {
		_sh_throw_error("efcapi_error_port.not.initialize");
		return "_efc_error";
	}

	if (_load_sipcfg(efc_Monitor, "ProtocolVersion") != efc_ver) {
		if (_efc_settings$void_protocol_version_check == false) {
			_sh_throw_error("efcapi_warning_protocol.not.the.same");
		}
	}

	return _load_sipcfg(efc_Monitor,portid);
}
