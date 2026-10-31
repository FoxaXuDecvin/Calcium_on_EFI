//RC API 
// Run Config API

//Copyright FoxaXu 2024C
// Based on OpenCppLangTab

#include"../shload.h"
#include"../Code/ThirdPartyCode.h"


int CL_FMV_ID = 4023;// Calcium Lang Format Version
//_$req_cl_fmv <Version>

int dfb_test = 1;
void _DEBUG_ForBreakpoint() {
	dfb_test++;
	dfb_test++;
	dfb_test++;
	return;
}

/// <VERSION>
//releaseVerKnd
const std::string _KV_rV_Debug = "1";
const std::string _KV_rV_Preview = "2";
const std::string _KV_rV_preRelease = "3";
const std::string _KV_rV_Demo = "4";
const std::string _KV_rV_Release = "5";
const std::string _KV_rV_Stable = "6";

//Other Version
const std::string _KV_rV_Custom = "7";
const std::string _KV_rV_Embed = "8";
const std::string _KV_rV_Deluxe = "9";

//Main Char
std::string _kv_text_debug = "Debug";
std::string _kv_text_preview = "Preview";
std::string _kv_text_prerelease = "Prerelease";
std::string _kv_text_demo = "Demo";
std::string _kv_text_release = "Release";
std::string _kv_text_stable = "stable";

//OV
std::string _kv_text_custom = "Custom";
std::string _kv_text_embed = "Embed";
std::string _kv_text_deluxe = "Deluxe";
//rVK END

//RunIDs
std::string _CK_Runid = _get_random_s(100000, 999999);

std::string _KV_softwareVersion = "121"; //(Software Version)

std::string _KV_gen = "4";//(General)

std::string _KV_rv = "3";//(Release Version)

std::string _KV_releaseVer = _KV_rV_Stable;//(Debug/Preview/preRelease/demo/Release  1 - 4)

std::string _mk = ".";

std::string _KernelVersion = _KV_softwareVersion + _mk + _KV_gen + _mk + _KV_rv + _mk + _KV_releaseVer;

//DEFINE
/// </VERSION>
/// 

void _KernelVersion_LoadText(void);
//Define

const std::string _rc_false = "false";
const std::string _rc_true = "true";

//carrige return,
const int _hex_cr = 00001101; // HEX 0D

// new line
const int _hex_nl = 00001010; // HEX 0A

//Build / Release / Prerelease   -  Release ID 
const std::string _RCapi_Version = "0";
std::string buildshell = "/calcium_settings.cfg";
std::string _shellTitle = "Calcium " + _KernelVersion + " Command Line>";

bool _isAdminOK_ = false;

//Rc Config var
bool _rcset_syscmd;
bool _rcset_allowdiredit;
bool _rcset_allowtp;
bool _rcset_anticrash;

bool _rcset_directmode;
bool _rcset_trustcheck;
bool _rcset_offlangcheck;

bool _rcset_shelledit,_rcset_scriptedit;

bool _rcset_enforceUTF8;

bool _rcset_aosr;

bool _rcset_useAdmin;

bool _rcset_LockRunningScript;

bool _BlockNotAllowEditConfig = false;



bool _rcset_crash_reload;

std::string _rcbind_pluginscript, _rcbind_pluginpath,_rcbind_thirdbind,_rcbind_autorun,_rcbind_autorunargs;

std::string _rcbind_langpath;
std::string _rcset_lang;
std::string _rcset_seclang;
std::string _rcbind_serverapi;
std::string _rc_exec_address;
std::string _pagefile_savedir;
std::string _OriginWorkDir;
int _exec_runtimesleep = 1;

bool _VarSpace_Random_BreakTest = false;
int VarSpaceRandomError = 0;

//Other
bool New_thread_auto_join = false;
bool Thread_Env = false;
std::thread AddNewScript;

void _pv(std::string info) {
	_p(_Old_VSAPI_TransVar(info));
	return;
}


std::string UserSetPATH = "nodata";
std::string TempPrc_Rcapi;
void _RcApi_vp_load(void) {
	
	if (UserSetPATH == "nodata") {
		TempPrc_Rcapi = _$GetSelfPath;
		TempPrc_Rcapi = ReplaceChar(TempPrc_Rcapi, "\\", "/");
		_varspaceadd("{path}", TempPrc_Rcapi);
		//_p("PATH USE DEFAULT :  " + TempPrc_Rcapi);
	}
	else {
		_varspaceadd("{path}", UserSetPATH);
		//_p("PATH USE USER :  " + TempPrc_Rcapi);
	}
	//_pause();
	_varspaceadd("{VersionID}", _KernelVersion);
}

std::string _RcApi_TapiBuffer;
bool _RcLoad_TransApi(std::string _Rc_ID) {
	_RcApi_TapiBuffer = _load_sipcfg_noreturn(buildshell, _Rc_ID);
	//_p("tapi Buffer :  " + _Rc_ID + "  " + _RcApi_TapiBuffer);
	if (_RcApi_TapiBuffer == _rc_true) {
		return true;
	}
	if (_RcApi_TapiBuffer == _rc_false) {
		return false;
	}

	_p("--------------------------------------------------------------------");
	_p("Null Config Set");
	_p("Setting :   " + _Rc_ID + "       Data :   \"" + _RcApi_TapiBuffer + "\"");
	_p("Data is not true or false");
	_p("Trans Api cannot identify \"" + _RcApi_TapiBuffer + "\"");
	_p("TransApi Return false");
	return false;
}

std::string url_cache;
bool _api_request_download(std::string Address,std::string Save) {
	url_cache = _rcbind_serverapi + "/" + Address;
	_p("api server :  " + _rcbind_serverapi);
	_p("request some files...   please wait");
	_p("url :  " + url_cache);
	if (!_urldown_api_nocache(url_cache, Save)){
		_p("URL Request Failed");
		return false;
	}
	return true;
}

bool _api_request_clear(std::string Address, std::string Save) {
	url_cache = _rcbind_serverapi + "/" + Address;
	if (!_urldown_api_nocache(url_cache, Save)) {
		return false;
	}
	return true;
}

bool _api_request_clear_cache(std::string Address, std::string Save) {
	url_cache = _rcbind_serverapi + "/" + Address;
	if (!_urldown_api(url_cache, Save)) {
		return false;
	}
	return true;
}


std::string file;
bool _direct_read_script = false;
int _gf_line_maxallow = 32768;
bool _RcApiLoadConfig() {
	//_p("Loading Config");
	file = buildshell;
	if (!check_file_existence(file)) {
		_p("Create New Config");
		_dapi_create_full_path(file);
		_soildwrite_open(file);
		_soildwrite_write(" //BuildShell SipCfg  --Use  true/false");
		_soildwrite_write("$CalciumVersion=" + _KernelVersion + ";");
		_soildwrite_write("$EnableSystemCommand=false;");
		_soildwrite_write("$EnableAntiCrash=false;");
		_soildwrite_write("$EnableCrashReload=true;");
		_soildwrite_write("$AllowDirectoryEdit=false;");
		_soildwrite_write("$AllowThirdPartyPlugin=false;");
		_soildwrite_write("$AllowShellEdit=true;");
		_soildwrite_write("$AllowScriptEdit=false;");
		_soildwrite_write("$UseDirectRead=false;");
		_soildwrite_write("$TrustedServerCheck=false;");
		_soildwrite_write("$OffLangCheck=true;");
		_soildwrite_write("$MaxScriptExecuteLine=2147483640;");
		_soildwrite_write("$AfterExecuteSleepTime=0;");
		_soildwrite_write("$VarSpaceRandomError=0;");
		_soildwrite_write("$LockingRunningScript=true;");
		_soildwrite_write("");
		_soildwrite_write("//ShellSettings");
		_soildwrite_write("$AutoOpenShellAfterRun=true;");
		_soildwrite_write("$UseSuperUser=false;");
		_soildwrite_write("");
		_soildwrite_write("//Bind");
		_soildwrite_write("$AutoRun=null;");
		_soildwrite_write("$AutoRunArgs=null;");
		_soildwrite_write("");
		_soildwrite_write("//PathBind");
		_soildwrite_write("$DefaultPluginPath={path}/Plugin;");
		_soildwrite_write("$DefaultPluginScript={path}/script;");
		_soildwrite_write("$DefaultLanguagePath={path}/lang;");
		_soildwrite_write("$PageFile={path}/envswap.tmp;");
		_soildwrite_write("$OriginWorkDir=" + TempPrc_Rcapi + ";");
		_soildwrite_write("");
		_soildwrite_write("//Display Settings");
		_soildwrite_write("$Language=en-us;");
		_soildwrite_write("$SecondLanguage=en-us;");
		_soildwrite_write("");
		_soildwrite_write("//Windows Only");
		_soildwrite_write("$EnforceUTF-8=false;");
		_soildwrite_write("");
		_soildwrite_write("//Server");
		_soildwrite_write("$RootAPIServer=http://githubimage.foxaxu.top;");
		_soildwrite_write("$ExecuteFile=" + _$GetSelfFull + ";");
		_soildwrite_write("");
		_soildwrite_write("$Configuration File Integrity Check=vaild;");
		_soildwrite_close();
	}

	//Check Integrity

	if (_load_sipcfg(file, "Configuration File Integrity Check") != "vaild") {
		//Failed to check config file
		if (_isAdminOK_ == true) {
			cleanConsole();
			_p("File: " + file);
			_p("Config file is not Integrity. or this config file not support your calcium version");
			_p("Delete this config file. calcium will create a new config file");
			_pause();
			return false;
		}
		_rcset_offlangcheck = true;
		_rcset_useAdmin = true;
		return true;
	}

	_rcset_syscmd = _RcLoad_TransApi("EnableSystemCommand");
	_rcset_anticrash = _RcLoad_TransApi("EnableAntiCrash");
	_rcset_crash_reload = _RcLoad_TransApi("EnableCrashReload");
	_rcset_allowdiredit = _RcLoad_TransApi("AllowDirectoryEdit");
	_rcset_allowtp = _RcLoad_TransApi("AllowThirdPartyPlugin");
	_rcset_shelledit = _RcLoad_TransApi("AllowShellEdit");
	_rcset_scriptedit = _RcLoad_TransApi("AllowScriptEdit");
	_rcset_directmode = _RcLoad_TransApi("UseDirectRead");
	_rcset_trustcheck = _RcLoad_TransApi("TrustedServerCheck");
	_rcset_offlangcheck = _RcLoad_TransApi("OffLangCheck");
	_gf_line_maxallow = atoi(_load_sipcfg_noreturn(file, "MaxScriptExecuteLine").c_str());
	_exec_runtimesleep = atoi(_load_sipcfg_noreturn(file, "AfterExecuteSleepTime").c_str());
	VarSpaceRandomError = atoi(_load_sipcfg_noreturn(file, "VarSpaceRandomError").c_str());
	_rcset_LockRunningScript = _RcLoad_TransApi("LockingRunningScript");

	_rcset_aosr = _RcLoad_TransApi("AutoOpenShellAfterRun");
	_rcset_useAdmin = _RcLoad_TransApi("UseSuperUser");

	//std::string
	_rcbind_autorun = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "AutoRun"));
	_rcbind_autorunargs = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "AutoRunArgs"));
	_rcbind_pluginpath = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "DefaultPluginPath"));
	_rcbind_pluginscript = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "DefaultPluginScript"));
	_rcbind_langpath = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "DefaultLanguagePath"));
	_pagefile_savedir = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "PageFile"));
	_OriginWorkDir = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "OriginWorkDir"));

	_rcset_lang = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "Language"));
	_rcset_seclang = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "SecondLanguage"));
	//Lang 
	_rcset_enforceUTF8 = _RcLoad_TransApi("EnforceUTF-8");

	_rcbind_serverapi = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "RootAPIServer"));
	_rc_exec_address = _Old_VSAPI_TransVar(_load_sipcfg_noreturn(file, "ExecuteFile"));

	//Create Directory
	if (!_rcset_offlangcheck) {
		if (!_dapi_ExistFolder_check(_rcbind_langpath)) {
			_dapi_create_full_path(_rcbind_langpath + "/a.txt");
		}
	}

	//Auto Set
	_direct_read_script = _rcset_directmode;

	//End

	_KernelVersion_LoadText();

	return true;
}


std::string fitback,fitbuffer;
std::string _Char_Filter_EndFileName(std::string fitchar) {
	fitchar = ReplaceChar(fitchar, "\\", "/");
	fitback = "";

	for (int fullsize = fitchar.size(); true; fullsize--) {
		fitbuffer = fitchar[fullsize];
		if (fitbuffer == "/") {
			fitback = cutendchar(fitback);
			return fitback;
		}

		fitback = fitbuffer + fitback;

	}

	return "LineError";
}


std::string cachecstp;
std::string filiter_mfh;
std::string cache_filepath;
std::string getfile;
std::string verifyGet;
std::string _cstp_file_write = "a.cstp";
int readptr = 1;
bool AntiDoubleCopy = false;
bool CstpOutput = true;

bool _$cstp_writeapiN(std::string file) {
	if (!check_file_existence(file)) {
		if (CstpOutput)_pv("Cstp _$lang.loadfail :  _$lang.filenotfound");
		return false;
	}
	cache_filepath = ReplaceChar(file, filiter_mfh, "");
	_soildwrite_write("$CSTP_FILE_RECORD_MARK_ID_START(" + cache_filepath + ");");

	for (int readptr = 1; true; readptr++) {
		cachecstp = LineReader(file, readptr);
		if (cachecstp == "overline") {
			if(CstpOutput)_p("End LoadScript. Total :  " + std::to_string(readptr));
			break;
		}
		if (cachecstp == "ReadFailed") {
			return false;
		}
		_soildwrite_write(cachecstp);
	}
	_soildwrite_write("$CSTP_FILE_RECORD_MARK_ID_END(" + cache_filepath + ");");
	return true;
}
bool _cstp_makerN(std::string make_file_header,std::string file) {
	CreateFileMap_txt("makedirmap.txt", make_file_header);
	_cstp_file_write = file;
	filiter_mfh = make_file_header;
	filiter_mfh = ReplaceChar(filiter_mfh, "\\", "/");
	filiter_mfh = ReplaceChar(filiter_mfh, "//", "/");
	make_file_header = "makedirmap.txt";
	_p("Execute Pack Directory " + _cstp_file_write);
	if (check_file_existence(_cstp_file_write)) {
		_fileapi_del(_cstp_file_write);
	}
	_soildwrite_open(_cstp_file_write);
	if (!check_file_existence(make_file_header)) {
		if (CstpOutput)_pv("Cstp Make failed :   _$lang.filenotfound");
		return false;
	}
	_soildwrite_write("Calcium_Text_Pack_File  New Format");
	_soildwrite_write("//START EXTRACT");
	for (int readptr = 1; true; readptr++) {
		getfile = cachecstp = LineReader(make_file_header, readptr);
		if (cachecstp == "overline") {
			break;
		}
		if (cachecstp == "ReadFailed") {
			break;
		}
		if (CstpOutput)_p("\r                                                             \r Add File :   " + cachecstp);
		if (_$cstp_writeapiN(cachecstp)) {
			//_pv("Cstp File :  " + getfile);
		}
		else {
			//_pv("Failed :  " + getfile);
		}
		continue;
	}
	if (CstpOutput)_pv("cstp make _$lang.complete");
	_fileapi_del("makedirmap.txt");
	_fileapi_del("empty.txt");
	return true;
}
bool _$cstp_unpackapiN(std::string file,std::string resourcefile,int startline,std::string extract_dir) {

	_dapi_create_full_path(extract_dir + "/" + file);
	readptr++;
	if (CstpOutput)std::cout << "\r                                                             \r Extract File :  " + file;
	if (check_file_existence(extract_dir + "/" + file)) {
		_fileapi_del(extract_dir + "/" + file);
	}
	creatpath(extract_dir + "/" + file);
	_soildwrite_open(extract_dir + "/" + file);
	for (; true; readptr++) {
		cachecstp = LineReader(resourcefile, readptr);
		if (cachecstp == "overline") {
			break;
		}
		if (cachecstp == "ReadFailed") {
			return false;
		}
		if (SizeRead(cachecstp, 29) == "$CSTP_FILE_RECORD_MARK_ID_END") {
			verifyGet = PartReadA(cachecstp, "(", ")", 1);
			if (verifyGet == getfile) {
				break;
			}
		}
		_soildwrite_write(cachecstp);
	}
	_soildwrite_close();
	if (!check_file_existence(extract_dir + "/" + file)) {
		_p("failed to write file " + file);
		return false;
	}
	else {
		return true;
	}

	return false;
}
bool _cstp_unpackN(std::string unpack_path, std::string file) {
	if (CstpOutput)_p("Extract Package " + file);
	if (!check_file_existence(file)) {
		if (CstpOutput)_pv("Cstp Unpack failed :   _$lang.filenotfound " + file);
		return false;
	}
	for (; true; readptr++) {
		getfile = cachecstp = LineReader(file, readptr);
		if (cachecstp == "overline") {
			break;
		}
		if (cachecstp == "ReadFailed") {
			break;
		}
			if (SizeRead(cachecstp, 31) == "$CSTP_FILE_RECORD_MARK_ID_START") {
				getfile = PartReadA(cachecstp, "(", ")", 1);
				if (!_$cstp_unpackapiN(getfile, file, readptr, unpack_path)) {
					if (CstpOutput)_pv("Failed :  " + getfile);
				}
		}
		continue;
	}
	if (CstpOutput)_pv("cstp unpack _$lang.complete");
	return true;
}


//Legacy CSTP
bool _Legacy_$cstp_writeapi(std::string file) {
	if (!check_file_existence(file)) {
		_pv("Cstp _$lang.loadfail :  _$lang.filenotfound");
		return false;
	}
	cache_filepath = ReplaceChar(file, filiter_mfh, "");
	_soildwrite_write("$sign_file_output(" + cache_filepath + ");");

	for (int readptr = 1; true; readptr++) {
		cachecstp = LineReader(file, readptr);
		if (cachecstp == "overline") {
			_p("End LoadScript. Total :  " + std::to_string(readptr));
			break;
		}
		if (cachecstp == "ReadFailed") {
			return false;
		}
		_soildwrite_write(cachecstp);
	}
	_soildwrite_write("$sign_file_close;");
	return true;
}
bool _Legacy_cstp_maker(std::string make_file_header, std::string file) {
	CreateFileMap_txt("makedirmap.txt", make_file_header);
	_cstp_file_write = file;
	filiter_mfh = make_file_header;
	filiter_mfh = ReplaceChar(filiter_mfh, "\\", "/");
	filiter_mfh = ReplaceChar(filiter_mfh, "//", "/");
	make_file_header = "makedirmap.txt";
	if (CstpOutput)_p("Execute Pack Directory " + _cstp_file_write);
	if (check_file_existence(_cstp_file_write)) {
		_fileapi_del(_cstp_file_write);
	}
	_soildwrite_open(_cstp_file_write);
	if (!check_file_existence(make_file_header)) {
		_pv("Cstp Make failed :   _$lang.filenotfound");
		return false;
	}
	_soildwrite_write("calcium_script_text_package");
	_soildwrite_write("//Text Package");
	for (int readptr = 1; true; readptr++) {
		getfile = cachecstp = LineReader(make_file_header, readptr);
		if (cachecstp == "overline") {
			break;
		}
		if (cachecstp == "ReadFailed") {
			break;
		}
		if (CstpOutput)_p("Add File :   " + cachecstp);
		if (_Legacy_$cstp_writeapi(cachecstp)) {
			if (CstpOutput)_pv("Cstp File :  " + getfile);
		}
		else {
			if (CstpOutput)_pv("Failed :  " + getfile);
		}
		continue;
	}
	if (CstpOutput)_pv("cstp make _$lang.complete");
	_fileapi_del("makedirmap.txt");
	_fileapi_del("empty.txt");
	return true;
}
bool _Legacy_$cstp_unpackapi(std::string file, std::string resourcefile, int startline, std::string extract_dir) {
	_dapi_create_full_path(extract_dir + "/" + file);
	readptr++;
	if (CstpOutput)std::cout << "\r                                                             \r Extract File :  " + file;
	if (check_file_existence(extract_dir + "/" + file)) {
		_fileapi_del(extract_dir + "/" + file);
	}
	creatpath(file);
	_soildwrite_open(extract_dir + "/" + file);
	for (; true; readptr++) {
		cachecstp = LineReader(resourcefile, readptr);
		if (cachecstp == "overline") {
			break;
		}
		if (cachecstp == "ReadFailed") {
			return false;
		}
		if (SizeRead(cachecstp, 16) == "$sign_file_close") {
			break;
		}
		_soildwrite_write(cachecstp);
	}
	_soildwrite_close();
	if (!check_file_existence(extract_dir + "/" + file)) {
		_p("failed to write file " + file);
		return false;
	}
	else {
		return true;
	}

	return false;
}
bool _Legacy_cstp_unpack(std::string unpack_path, std::string file) {
	if (CstpOutput)_p("Extract Package " + file);
	if (!check_file_existence(file)) {
		_pv("Cstp Unpack failed :   _$lang.filenotfound " + file);
		return false;
	}
	for (; true; readptr++) {
		getfile = cachecstp = LineReader(file, readptr);
		if (cachecstp == "overline") {
			break;
		}
		if (cachecstp == "ReadFailed") {
			break;
		}
		if (SizeRead(cachecstp, 17) == "$sign_file_output") {
			getfile = PartReadA(cachecstp, "(", ")", 1);
			if (!_Legacy_$cstp_unpackapi(getfile, file, readptr, unpack_path)) {
				_pv("Failed :  " + getfile);
			}
		}
		continue;
	}
	_pv("cstp unpack _$lang.complete");
	return true;
}

std::string dircache;
bool _packsetup(std::string packid) {
	dircache = _$GetSelfPath + "/Setup_" + _get_random_s(1,10000);
	if (!_api_request_download(packid,dircache)) {
		if (!_api_request_clear_cache(packid, dircache)) {
			_pv("_packsetup _$lang.fail :  " + packid);
			return false;
		}
	}

	_Legacy_cstp_unpack(_rcbind_pluginscript, dircache);
	_p("complete ...");

	_fileapi_del(dircache);

	return true;
}

int tbd_year;
int tbd_month;
int tbd_day;
bool _TBD_WARNING;
std::string TBD_STR;
std::string TBD_cache;
bool _Time_Bomb_Detect(std::string CurrentRV) {
	return false;
}


std::string at_cache;
bool _TrustedServer;
bool _activate_calcium(std::string Key_Register) {
	return true;
}

bool _activate_request(std::string key_reg) {
	return true;
}

void _var_typetext(std::string file) {
	if (_existfile(file)) {
		int maxdocsize = getdocmax(file);

		maxdocsize++;

		for (int startRoll = 1; maxdocsize != startRoll; startRoll++) {
			_p(_Old_VSAPI_TransVar(_fileapi_textread(file, startRoll)));
		}
		return;
	}
	else {
		_sh_throw_error("FileAPI.TypeText :  File Not Exist");
		return;
	}
}

void _Create_Analysis_File(std::string savefile) {
	_soildwrite_open(savefile);
	_soildwrite_write("::Calcium Analysis File");
	_soildwrite_write("::Copyright FoxaXu " + $year_message);
	_soildwrite_write("::Kernel Message");
	_soildwrite_write("-Execute File Path :   " + _$GetSelfFull);
	_soildwrite_write("-System :  " + _Run_SysKernel);
	_soildwrite_write("-Calcium Activate Status :  " + std::to_string(_kernel_activate));
	_soildwrite_write("-Language :  " + _rcset_lang + "    Second Language :  " + _rcset_seclang);
	_soildwrite_write("-Version :  " + _KernelVersion);
	_soildwrite_write("-Created Time :  " + __GetFullTime());
	_soildwrite_write("");
	_soildwrite_write("");
	_soildwrite_write("");
	_soildwrite_write("----Project ID----");
	_soildwrite_write("-CID  :    " + _CTitle);
	_soildwrite_write("-VerText :     " + _KV_rV_Text);
	_soildwrite_write("-InsideVersion :     " + InsideVersion);
	_soildwrite_write("----OpenCppLangTab----");
	_soildwrite_write("-OCLT ID :  " + $version_title);
	_soildwrite_write("-OCLT Release :  " + $version_msg);
	_soildwrite_write("-OCLT CodeName :   " + $codename);
	_soildwrite_write("-OCLT RID :    " + $version_code_str);
	_soildwrite_write("-OCLT BuildTime :  " + $buildtime);
	_soildwrite_write("+ Debug ++++++++++++++++++++++++++++++++");
	_soildwrite_write("");
	_soildwrite_write("---- VarSpace Map-----");
	_soildwrite_write(VarSpace);
	_soildwrite_write("----End varspace ----");
	_soildwrite_write("");
	_soildwrite_write("----Execute Address----");
	_soildwrite_write("Script File :  " + _global_scriptload + " Line :  " + std::to_string(_gf_line) + "  breakpoint :  " + std::to_string(_gf_cg));
	_soildwrite_write("---- End address----");
	_soildwrite_write("");
}

int ExecCache;
std::string _PluginExecuteAPI(std::string execplugin,std::string script,int address) {
	if (!check_file_existenceA(execplugin)) {
		execplugin = _rcbind_pluginpath + "/" + execplugin;
			if (!check_file_existenceA(execplugin)) {
				_p("Fail load error:  File not Exist");
				_p(execplugin);
				return "pluginnotfound";
			}
	}

	ExecCache = _system_autoRun(execplugin, _$quo + script + _$quo + " " + std::to_string(address));


}

//New Goto API
// -4 NotFound
int fal_cache,cd_cache;
std::string fal_buffer;
int _FindAllLine(int startline, int skipline, std::string file, std::string charData) {
	//_p("FAL Debug:   File:  <" + file + ">   Data:  <" + charData + ">");
	cd_cache = charData.size();
	fal_cache = FindCharLineA(startline, skipline, file, charData);
	if (fal_cache == -4) return -4;

	fal_buffer = LineReader(file, fal_cache);
	fal_buffer = HeadSpaceClean_NoSEM(fal_buffer);

	if (SizeRead(fal_buffer,cd_cache) != charData) while (true) {
		fal_cache++;
		fal_cache = FindCharLineA(fal_cache, skipline, file, charData);
		if (fal_cache == -4) return -4;
		fal_buffer = LineReader(file, fal_cache);
		fal_buffer = HeadSpaceClean_NoSEM(fal_buffer);
		if (SizeRead(fal_buffer, cd_cache) != charData) continue;
		break;
	}

	//_p("FAL Debug:   return value :  <" + std::to_string(fal_cache)+">");
	return fal_cache;
}

int FC_A, FC_B, CP_POINT;
std::string CP_CACHE_A, CP_CACHE_B;
bool FileCompare_(std::string fileA, std::string fileB) {

	if (!check_file_existence(fileA))return false;
	if (!check_file_existence(fileB))return false;

	FC_A = _textapi_getdocmax(fileA);
	FC_B = _textapi_getdocmax(fileB);

	if (FC_A != FC_B)return false;

	CP_POINT = 1;

	for (; 1; CP_POINT++) {

		CP_CACHE_A = _fileapi_textread(fileA, CP_POINT);
		CP_CACHE_B = _fileapi_textread(fileB, CP_POINT);

		if (CP_CACHE_A == "badread") {
			return false;
		}
		if (CP_CACHE_A == "badParameter") {
			return false;
		}
		if (CP_CACHE_A == "badopen") {
			return false;
		}

		if (CP_CACHE_B == "badread") {
			return false;
		}
		if (CP_CACHE_B == "badParameter") {
			return false;
		}
		if (CP_CACHE_B == "badopen") {
			return false;
		}

		//Check Over
		if (CP_CACHE_A == CP_CACHE_B) {
			if (CP_CACHE_A == "overline") return true;
		}

		if (CP_CACHE_A != CP_CACHE_B) return false;

		continue;
	}
	//P2
}

std::string CharFilter_(std::string msg) {
	msg = ReplaceChar(msg, "\\r", "\r");

	msg = ReplaceChar(msg, "\\t", "\t");

	msg = ReplaceChar(msg, "\\n", "\n");

	msg = ReplaceChar(msg, "\\b", "\b");

	return msg;
}

//DiskPerfTest

bool SWTTStop = false;
int TotalWrtSize;
int SpeedWriteTestThread() {
	//File WriteStream
	while (SWTTStop == false) {
		_fileapi_write(_$GetSelfPath + "/bmwf.tmp", "This is Calcium File Write Performance Test");
		TotalWrtSize++;
	}

	return 0;
}



int FileWriteSpeedTest() {
	SWTTStop = false;
	TotalWrtSize = 1;
	_fileapi_write(_$GetSelfPath + "/bmwf.tmp","Start BenchMark");
	std::thread SpeedWrite(SpeedWriteTestThread);
	SpeedWrite.detach();
	sleepapi_ms(3000);
	SWTTStop = true;
	
	sleepapi_ms(1000);
	_fileapi_del(_$GetSelfPath + "/bmwf.tmp");

	TotalWrtSize = TotalWrtSize / 3;

	return TotalWrtSize;
}

//COmmand ProcessAPI

bool CMDPROCStop = false;
int TotalProcSize;
int OperateSize, OSA, OSB;

int FileCmdProcThread() {
	while(CMDPROCStop == false) {
		LineReader(_$GetSelfPath + "/bmwf.tmp", 1);
		OperateSize = _get_random(1, 100);
		OSA = _get_random(1, 100);
		OSB = _get_random(1, 100);
		OperateSize = OperateSize * OSA;
		OperateSize = OperateSize * OSB;
		TotalProcSize++;
	}
	return 0;
}


int FileCmdProcessSpeedTest() {
	_fileapi_write(_$GetSelfPath + "/bmwf.tmp","Start Analysis Test: OOOOOOOOOOOOOOOOOOOOQQQQQQQQQQQQQQQQQQQQQQAAAAAAAAAAAAAAAAAA");

	CMDPROCStop = false;
	TotalProcSize = 0;

	std::thread ProcessCmd(FileCmdProcThread);
	ProcessCmd.detach();
	sleepapi_ms(3000);
	CMDPROCStop = true;
	sleepapi_ms(1000);

	_fileapi_del(_$GetSelfPath + "/bmwf.tmp");

	TotalProcSize = TotalProcSize / 3;

	return TotalProcSize;
}



int CommandSpeed_CountNum = 0;
int LastCache_TPC = 0;
int TotalCommandExec_TPC = 0;

int VarSpacePT_IO = 0;
int Last_VarSpacePT_IO = 0;

int LastCache_TPD = 0;
int TotalCE_TPD = 0;

int MAX_TPC, MAX_TPD;

bool ProcessReqStop = false;
bool is_TPC_already_Running = false;
bool TPC_all_exit = true;
std::string PerfCNT_ID,PerfCNT_File,Address_TrackFile,Error_TrackFile;
int Thread_PerfCurrentGet() {
	//StartLE

	Address_TrackFile = _$GetSelfPath + "/StepTrack " + PerfCNT_ID + ".txt";
	Error_TrackFile = _$GetSelfPath + "/TotalErrorHappend " + PerfCNT_ID + ".txt";

	LastCache_TPC = 0;
	TPC_all_exit = false;
	ProcessReqStop = false;
	TotalCommandExec_TPC = 0;
	CommandSpeed_CountNum = 0;
	is_TPC_already_Running = true;

	LastCache_TPD = 0;
	TotalCE_TPD = 0;

	MAX_TPC = 0;
	MAX_TPD = 0;

	CommandSpeed_CountNum = 0;
	LRBuffer_Count = 0;

	while (1) {
		sleepapi_ms(1000);
		//CPU
		LastCache_TPC = CommandSpeed_CountNum;
		CommandSpeed_CountNum = 0;
		TotalCommandExec_TPC = TotalCommandExec_TPC + LastCache_TPC;
		if (MAX_TPC < LastCache_TPC)MAX_TPC = LastCache_TPC;

		LastCache_TPD = LRBuffer_Count;
		LRBuffer_Count = 0;

		TotalCE_TPD = TotalCE_TPD + LastCache_TPD;
		if (MAX_TPD < LastCache_TPD)MAX_TPD = LastCache_TPD;

		Last_VarSpacePT_IO = VarSpacePT_IO;
		VarSpacePT_IO = 0;

		PerfCNT_File = _$GetSelfPath + "/api_cpspeed_id " + PerfCNT_ID + ".txt";

		_fileapi_del(PerfCNT_File);
		_fileapi_write(PerfCNT_File, "Calcium Performance Record Tool");
		_fileapi_write(PerfCNT_File, "Current Command Speed :   " + std::to_string(LastCache_TPC) + " / Every second     MaxSpeed:  " + std::to_string(MAX_TPC));
		_fileapi_write(PerfCNT_File, "Total Execute Command :    " + std::to_string(TotalCommandExec_TPC));
		_fileapi_write(PerfCNT_File, " ");
		_fileapi_write(PerfCNT_File, "Disk IO :   " + std::to_string(LastCache_TPD) + " / Every second     MaxSpeed:  " + std::to_string(MAX_TPD));
		_fileapi_write(PerfCNT_File, "Total Disk CR :    " + std::to_string(TotalCE_TPD));
		_fileapi_write(PerfCNT_File, " ");
		_fileapi_write(PerfCNT_File, "Var Space IO :   Total (Include add/modify/remove) : " + std::to_string(Last_VarSpacePT_IO));
		_fileapi_write(PerfCNT_File, "Var Space Total  :    " + std::to_string(VarSpaceMax) +   "       Memory Usage :   " + std::to_string(VarSpace.size()) + " Byte");
		_fileapi_write(PerfCNT_File, " ");
		_fileapi_write(PerfCNT_File, "Processing address.  File :  " + _global_scriptload + " Line :  " + std::to_string(_gf_line) + "  breakpoint :  " + std::to_string(_gf_cg));
		if (ProcessReqStop == true) break;
		
		continue;
	}

	is_TPC_already_Running = false;
	_fileapi_del(PerfCNT_File);
	_fileapi_del(Address_TrackFile);
	_fileapi_del(Error_TrackFile);
	TPC_all_exit = true;

	return 0;
}

void HelpArgsType() {
	_p("OpenCalcium Platform Argument");
	_p(" ");
	_p(" Basic Argument");
	_p("     -run  {ScriptFile}        Run Script");
	_p("     -shell                          Open Calcium Shell");
	_p("     -args {Argument}      Run Script with argument");
	_p(" ");
	_p(" Advanced Argument");
	_p("     -noeditconfig     Reject all \"calcium_settings.cfg\" modify");
	_p("     -nolang               Skip Load Language resources");
	_p("     -runid {RunID}                  Run ID (Thread name)");
	_p("     -fastmode            Use Fast Mode to Load Calcium (No Language. No Resource Load) ");
	_p(" ");
	_p(" Developer Argument");
	_p("     --enable-cmdtrack                 Enable Command Tracker and write to file");
	_p("     --varspace-modifytrack         Enable track  all environment variable modify operate");
	_p("     --stack-tracker                        Enable Script Stack Tracker");
	_p("     -loadenv   {File}                      Load Saved Environment to Var Space");
	_p("     -enable-vs-random-break   To Debug Error Recovery. Choose random var replace to '?'.emulate error happend.");
	_p("     --perf-test                              Run Calcium Performance Test");
	_p("     -perfid {ID}                             TPC Service. Track Performance Active");
	_p("");
	return;
}

//NETWORKAPI
bool NAT_Result, NAT_isTimeouted, NAT_Status;
int NAT_TimeClock;
int NetworkAgent_Thread(std::string URL, std::string SavePath) {
	NAT_Result = _urldown_api_nocache(URL, SavePath);
	NAT_Status = true;
	if (NAT_Result == true) {
		if (NAT_isTimeouted == true) {
			_fileapi_del(SavePath);
			return 2;
		}
		return 0;
	}
	else {
		return 3;
	}
}

std::string NewWebDownloadSocket(std::string URL, std::string SavePath) {
	//Add Process
	NAT_Result = NAT_isTimeouted = NAT_Status = false;

	std::thread networkio_download(NetworkAgent_Thread, URL, SavePath);
	networkio_download.detach();

	NAT_TimeClock = Network_AgentWaitTime;
	while (true) {
		if (NAT_Status == true) break;
		sleepapi(1);
		NAT_TimeClock--;
		if (NAT_TimeClock <= 0) break;
	}

	if (NAT_Status == false) {
		NAT_isTimeouted = true;
		sleepapi_ms(200);
		_fileapi_del(SavePath);
		return "CA_CONNECT.TIMEOUTED";
	}
	if (NAT_Result == true) {
		return "true";
	}

	if (NAT_Result == false) {
		return "CA_CONNECT.FAILED";
	}

	return "CA_CONNECT.INVAILD_CODE";
}

bool A_NewWebDownloadSocket(std::string URL, std::string SavePath) {
	//Add Process
	NAT_Result = NAT_isTimeouted = NAT_Status = false;

	std::thread networkio_download_A(NetworkAgent_Thread, URL, SavePath);
	networkio_download_A.detach();

	NAT_TimeClock = Network_AgentWaitTime;
	while (true) {
		if (NAT_Status == true) break;
		sleepapi(1);
		NAT_TimeClock--;
		if (NAT_TimeClock <= 0) break;
	}

	if (NAT_Status == false) {
		NAT_isTimeouted = true;
		sleepapi_ms(200);
		_fileapi_del(SavePath);
		return false;
	}
	if (NAT_Result == true) {
		return true;
	}

	if (NAT_Result == false) {
		return false;
	}

	return false;
}