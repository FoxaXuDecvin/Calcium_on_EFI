#pragma once

#include"../shload.h"
#include"../Code/CalciumKernel.h"

#include<signal.h>

bool PreLaunchLoad(void) {
	//Put Preload code here
	//
	
	//Time Bomb
	tbd_year = 2025;
	tbd_month = 10;
	tbd_day = 26;


	//PreLoad
	_CTitle = "Calcium_Project_Rebuild";

	__settings_displaylaunchscreen = false;

	//End
	return true;
}

void TypeHelpMenu() {
	_p("----------------------------------------------------------");
	_p("OpenCppLangTab  " + $version_title + " " + $version_code_str);
	_p("System :    " + _Run_SysKernel);
	_p("----------------------------------------------------------");
	_p("Kernel Version :    " + _KernelVersion + "  " + _KV_rV_Text);
	_p("CLFMV Version :     " + std::to_string(CL_FMV_ID));
	_p("OpenCalcium Script Platform    .....");
	_p("Github : https://github.com/FoxaXuDecvin/Calcium_Project_Code_VS2026");
	_p("Language   " + _rcset_lang);
	_p("Copyright FoxaXu " + $year_message);
	return;
}


//Args API
void argsApi(std::string args$api) {

	//if (argsSetExit) set true
	//Process will close after args run

	//Args Switch
	if (args$api == "-helpargs") {
		_runmode = _runmode_helpargs;
		return;
	}

	if (args$api == "-run") {
		_setnextargs_runscript = true;
		_runmode = _runmode_runscript;
		return;
	}

	if (args$api == "-optimi") {
		_setnextargs_runscript = true;
		_runmode = _runmode_optimi;
		return;
	}

	if (args$api == "-pack") {
		_setnextargs_runscript = true;
		_runmode = _runmode_cstpmake;
		return;
	}

	if (args$api == "-unpack") {
		_setnextargs_runscript = true;
		_runmode = _runmode_cstpunpack;
		return;
	}

	if (args$api == "-to") {
		_setnextargs_addo = true;
		return;
	}

	if (args$api == "-debug") {
		_debugMode = true;
		return;
	}

	if (args$api == "-help") {
		_runmode = _runmode_typehelp;
		return;
	}

	if (args$api == "-shell") {
		_runmode = _runmode_openshell;
		return;
	}

	if (args$api == "-version") {
		_runmode = _runmode_listversion;
		return;
	}

	if (args$api == "-anticrash_ok") {
		_anticrash_services = true;
	}

	if (args$api == "-args") {
		_setnextargs_addargs = true;
		return;
	}

	if (args$api == "-direct") {
		_direct_read_script = true;
	}

	if (args$api == "-noeditconfig") {
		_BlockNotAllowEditConfig = true;
	}

	if (args$api == "-path") {
		_setproces_path = true;
		return;
	}

	if (args$api == "-nolang") {
		_skipcheck_language = true;
	}

	if (args$api == "--enable-cmdtrack") {
		_CommandTracker = true;
		_fileapi_createmark("CommandTracker.txt", "Tracker Report File");
	}

	if (args$api == "--varspace-modifytrack") {
		_VarSpaceMFTracker = true;
		_fileapi_createmark("VarSpace_ModifyTracker.txt", "Tracker Report File");
	}

	if (args$api == "--stack-tracker") {
		_StackTracker = true;
		_fileapi_del(_StackTrackerFile);
		_fileapi_createmark(_StackTrackerFile, "PROCESS SCRIPT.STACK LIST");
		_write_sipcfg(_StackTrackerFile, "Stack_Size_Total", "0");
		_fileapi_write(_StackTrackerFile, " ");
		return;
	}

	if (args$api == "-loadenv") {
		_setnextargs_loadenv = true;
		return;
	}

	if (args$api == "-runid") {
		_setproces_runid = true;
		return;
	}

	if (args$api == "-%NowIsRunAdmin%") {
		_isAdminOK_ = true;
	}

	if (args$api == "-fastmode") {
		_FastMode = true;
	}

	if (args$api == "-enable-vs-randombreak") {
		_VarSpace_Random_BreakTest = true;
	}

	if (args$api == "--perf-test") {

		_p("Disk Performance Test Result is :  " + std::to_string(FileWriteSpeedTest()));
		_p("Command Performance Test Result is :  " + std::to_string(FileCmdProcessSpeedTest()));
		_p("Benchmark Version :  " + _KernelVersion + "    Platform :  " + _Run_SysKernel);

		argsSetExit = true;

		return;
	}

	if (args$api == "-perfid") {
		_setproces_perfid = true;
		return;
	}

	//auto set args
	if (_setnextargs_runscript == true) {
		runscript = args$api;
		_setnextargs_runscript = false;
	}

	if (_setnextargs_addargs == true) {
		script_args = args$api;
		_setnextargs_addargs = false;
	}

	if (_setnextargs_loadenv == true) {
		_setnextargs_loadenv = false;
		charCutB = args$api;

		if (!check_file_existence(charCutB)) {
			_p("EnvLoad Error Fail");
			_p("FileNotExist");
			return;
		}

		VarSpaceMax = atoi(_fileapi_textread(charCutB, 1).c_str());
		VarSpace = _fileapi_textread(charCutB, 2);
		return;
	}

	if (_setnextargs_addo == true) {
		o_info = args$api;
		_setnextargs_addo = false;
	}

	if (_setproces_runid == true) {
		Reg_Proces_runid = args$api;
		_setproces_runid = false;
	}

	if (_setproces_perfid == true) {
		PerfCNT_ID = args$api;

		if (PerfCNT_ID == "unknown.command")PerfCNT_ID = "Unnamed_" + _get_random_s(111111, 999999);

		std::thread monitor_pfr(Thread_PerfCurrentGet);
		monitor_pfr.detach();

		_setproces_perfid = false;
	}

	if (_setproces_path == true) {
		UserSetPATH = args$api;
		_setproces_path = false;
	}


	return;
}

std::string _user_typebuffer;
bool CK_Shell_open(void) {
	_global_scriptload = "{ShellMode}";
	cc_script_privilege = 0;
	if (cc_script_privilege == -1) {
		_p("OpenShell failed. privilege is not inited");
		_pause();
		return false;
	}
	_CK_ShellMode = true;
	_pn();
	_pn();
	_p("----------------------------------------------------------");
	_pv("_$lang.language :  _$lang.about .....  " + _rcset_lang);
	_p("type command,   use \"_exit\" to exit.");

	while (true) {
		_global_scriptload = "{ShellMode}";
		_pn();
		_prts(_shellTitle);
		_user_typebuffer = _getline_type() + ";";



		last_return = _api_result = _runcode_api(_user_typebuffer);

		if (_api_result == "runid.exit") {
			_pn();
			_p("Exit Calcium Shell Console");
			return true;
		}

		if (_api_result == "runid.entershell") {
			_p("The current environment is in Shell Mode");
		}

		if (_api_result == "runid.crash.harddrv.error") {
			_p("[Error] Unexpected Error. File Location was Broken");
			_p("[Error] Please check your hard drive or file system.");
		}

		if (_stop_exec_script == true) {
			if (_shell_lock) {
				_pn();
				_p("  -Stop_exec_Script is true  :   return data = " + _api_result);
				_stop_exec_script = false;
			}
			else {
				_pn();
				_p("  -Shell Closed :   return data = " + _api_result);
				return 0;
			}
		}

		continue;
	}


	return true;
}


//ATEXIT RegOut Exit
void regout_atexit_noparameter() {
	_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
	return;
}

void regout_atexit(int TNum) {
	_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
	exit(TNum);
}


int AntiCrash_Return_Code;
std::string ckapi_result;
std::string langpackfile;
std::string AC_FAILCODE = "{Null}";
//Put Code Here
std::string listen_id_tm_manager;
int ReturnResultCode;
bool _TaskLiveDetect;

int _ThreadManager_HeadMainLoad();
int ftLoadMainCodeCpp();
bool _void_ToAntiCrash;
int _HeadMainLoad() {
	listen_id_tm_manager = "wait_to_set" + _get_random_s(10000000,99999999);
	_TaskLiveDetect = true;
	std::thread taskmain(ftLoadMainCodeCpp);
	taskmain.detach();
	//Thread Manager
	//Wait Init
	__settings_throwErrorMode = false;

	while(1) {
		if (_TaskLiveDetect == false) {
			return ReturnResultCode;
		}
		if (_load_sipcfg(Reg_Process_Map, listen_id_tm_manager) == "alive") {
			break;
		}
		//Resync ID
		listen_id_tm_manager = Reg_Proces_runid;
		sleepapi_ms(100);
	}

	//Thread Load Succeed
	while (1) {
		if (_TaskLiveDetect == false) {
			_remove_sipcfg(Reg_Process_Map, listen_id_tm_manager);
			return ReturnResultCode;
		}
		if (_load_sipcfg(Reg_Process_Map, listen_id_tm_manager) == "killed") {
			_remove_sipcfg(Reg_Process_Map, listen_id_tm_manager);
			break;
		}

		//Resync ID
		listen_id_tm_manager = Reg_Proces_runid;
		sleepapi_ms(300);
	}
	return ReturnResultCode;
}

int ftLoadMainCodeCpp() {
	//_p("Calcium Thread Manager :   Load MainThread");
	ReturnResultCode = _ThreadManager_HeadMainLoad();
	_TaskLiveDetect = false;
	//_p("Calcium Thread Manager :   task close program");
	return 0;
}

int _ThreadManager_HeadMainLoad() {
	//CopyVersion
	if (SizeRead(_$GetSelfPath, 2) == "\\\\") {
		_p("Network Drive Not Support");
		_p("Calcium Running on Network Path");
		_p("But Windows Command is not support Network Path");
		_p("Please use Network mapping and running on mapping path");
		_p("");
		_p("不支持的网络驱动器");
		_p("Calcium 正在运行在网络驱动器");
		_p("Console 无法在当前路径运行");
		_p("为保证兼容性请使用本地路径或将当前路径映射到本地运行 ");
		_pause();
		return 0;
	}

	//_p("Use local dir");
	buildshell = _$GetSelfPath + "/calcium_settings.cfg";

	if (!_dapi_ExistFolder_check(_$GetSelfPath)) {
		_p("Unable to access the currently running directory");
		_p("Calcium cannot be run in the current directory");
		_p("无法访问Calcium运行的目录");
		_p("请尝试将Calcium运行在其它目录或使用管理员权限运行");
		_pause();
		return 1;
	}
	if (_anticrash_services == false) {
		//History Code
	}

	_RcApi_vp_load();
	if (!_RcApiLoadConfig()) {
		_p("Failed to Load RCapi.");
		_p("Config file is missing :  " + buildshell);
		_p("try to repair and try again.");
		_pause();
		return -1;
	}

	if (!_FastMode) {
		if (_rcset_offlangcheck) {
			_skipcheck_language = true;
		}
		else {
			if (_skipcheck_language == false) {
				if (!LanguageLoad()) {
						langpackfile = _$GetSelfPath + "/" + "temp_languagepack.pack";
						_p("Download Language files");

						if (_Run_SysKernel == Linux_kernel) {
							_p("Use Linux Language");
							if (!_api_request_download("lang/linux.txt", langpackfile)) {
								_p("Install Failed");
							}
						}
						if (_Run_SysKernel == Win32_kernel) {
							_p("Use Windows Language");
							if (!_api_request_download("lang/win.txt", langpackfile)) {
								_p("Install Failed");
							}
						}
						_system_autoRun(_$GetSelfFull, "-nolang -unpack \"" + langpackfile + "\" -to \"" + _rcbind_langpath + "\"");
						_p("Complete Install Language");
						_fileapi_del(langpackfile);
						sleepapi(1);
						cleanConsole();

						LanguageLoad();
				}
			}
		}
	}

	if (_Run_SysKernel == Win32_kernel) {
		//Execute Code
		if (_rcset_enforceUTF8 == true) {
			system("chcp 65001 >nul");
		}
	}

	_gf_cg = 0;
	_gf_cgmax = 1;
	_gf_line = 1;
	_gf_charget = "";
	
	if (!_FastMode) {
		if (check_file_existence(_$GetSelfPath + "/Calcium.pdb")) {
			//Debug Mode
			_rcset_anticrash = false;
			_p("[Calcium.pdb] Found PDB File.  No Crash Defender");
		}

		if (_rcset_useAdmin) {
			if (!_isAdminOK_) {
				_system_autoRun_admin(_$GetSelfFull, native_argument + " -%NowIsRunAdmin%");
				return 0;
			}
		}

		if (_rcset_anticrash == true) {
			if (_anticrash_services == false) {
			Crash_Reload_service:
				_void_ToAntiCrash = true;
				AntiCrash_Return_Code = _system_autoRun(_$GetSelfFull, native_argument + " \"-anticrash_ok\"");
				if (AntiCrash_Return_Code == 0) {
					_TaskLiveDetect = false;
					return 0;
				}

				AC_FAILCODE = _Old_VSAPI_TransVar("_erc_" + std::to_string(AntiCrash_Return_Code));

				if (AC_FAILCODE == "_erc_" + std::to_string(AntiCrash_Return_Code)) {
					AC_FAILCODE = "UNKNOWN_KERNEL_STATUS";
				}

				cleanConsole();
				_pn();
				_pv("_$lang.crash.title");
				_pn();
				_pv("     _$lang.crash.t1");
				_pv("       _$lang.crash.t2");
				_pv("       _$lang.crash.t3");
				_pv("       _$lang.crash.errcode --> " + AC_FAILCODE);
				_pv("       _$lang.crash.kcode :  " + std::to_string(AntiCrash_Return_Code));
				_pv("     _$lang.runargs :  " + native_argument);
				_pn();
				_pv("  _$lang.presskey");
				_pv("  _$lang.crash.report");
				_pn();
				_pn();
				_pause();
				if (_rcset_crash_reload) {
					cleanConsole();
					goto Crash_Reload_service;
				}
				else {
					_TaskLiveDetect = false;
					return 0;
				}
			}
		}
		else {
			//_p("Anti Crash Services is " + to_string(_rcset_anticrash));
		}


		if (_rcbind_autorun != "null") {
			_runmode = _runmode_runscript;
			runscript = _rcbind_autorun;
			script_args = _rcbind_autorunargs;

			ckapi_result = _ckapi_scriptload(runscript, script_args);
			if (ckapi_result == "runid.entershell") {
				_runmode = _runmode_openshell;
			}
			else {
				return 0;
			}
		}

		if (_debugMode == true) {
			_p("Runmode ID " + _runmode);
			_p("runscript :  " + runscript);
			_p("Argument :  " + script_args);
			_p("o info :  " + o_info);
			_pause();
			return 0;
		}
	}

	if (Reg_Proces_runid == "NoNameProcess") {
		Reg_Proces_runid = _get_random_s(10000000, 99999999) + "~DiaReg";
	}

	//Register Session Dialogue
	
	Reg_Process_Map = _OriginWorkDir + "/session_map.txt";
	_$GetSelfPath = _OriginWorkDir;
	if (_OriginWorkDir == "voidcheck") {

	}
	else {
		_varspaceadd("{path}", _OriginWorkDir);
	}

	if (!check_file_existence(Reg_Process_Map)) {
		_fileapi_write(Reg_Process_Map, "//  Calcium Dialogue Register");
	}

	//_p("Register on " + Reg_Process_Map + "   ID " + Reg_Proces_runid);
	//Config at process Exit 
	// 
	
	signal(SIGINT, regout_atexit);
	signal(SIGTERM, regout_atexit);
	signal(SIGILL, regout_atexit);
	signal(SIGFPE, regout_atexit);
	signal(SIGSEGV, regout_atexit);
 	signal(SIGABRT, regout_atexit);

	//main

	_write_sipcfg(Reg_Process_Map, Reg_Proces_runid, "alive"); //Delay Report Status

	if (_runmode == _runmode_null) {
		TypeHelpMenu();
		_pause();
		if (_rcset_aosr == true) {
			_runmode = _runmode_openshell;
		}
		else {
			_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
			return 0;
		}
	}

	if (_runmode == _runmode_helpargs) {
		HelpArgsType();
		_pause();
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}

	if (_runmode == _runmode_optimi) {
		readbufferA = runscript + "_optimi.ca";
		_p("Optimi Mode");
		if (runscript == "{null}") {
			_p("Command :  <program.exe> -optimi <file>");
			_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
			return 0;
		}
		_p("Optimi Script :  " + runscript);
		_p("Output Script :  " + readbufferA);
		if (!check_file_existence(runscript)) {
			_p("File Not Found :  " + runscript);
			_p("Command :  <program.exe> -optimi <file>");
			_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
			return 0;
		}
		_soildwrite_open(readbufferA);
		_soildwrite_write("_$directmode;");
		_soildwrite_write("_$nolog;");
		while(true) {
			charCutA = _get_fullLine(runscript, ";");
			_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
			if (_gf_status == false) break;
			_soildwrite_write(charCutA);
		}
		_soildwrite_close();

		_p("Optimi Complete");
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}
	if (_runmode == _runmode_cstpmake) {
		_Legacy_cstp_maker(runscript, o_info);
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}
	if (_runmode == _runmode_cstpunpack) {
		_Legacy_cstp_unpack(o_info,runscript);
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}
	if (_runmode == _runmode_typehelp) {
		TypeHelpMenu();
		_pause();
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}
	if (_runmode == _runmode_autoexit) {
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}
	if (_runmode == _runmode_runscript) {
		cc_script_privilege = 0;
		ckapi_result = _ckapi_scriptload(runscript, script_args);
		if (ckapi_result == "runid.crash.harddrv.error") {
			return 25581;
		}
		if (ckapi_result == "runid.entershell") {
			_runmode = _runmode_openshell;
		}
		else {
			_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
			return 0;
		}
		

	}
	if (_runmode == _runmode_openshell) {
		CK_Shell_open();
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}
	if (_runmode == _runmode_listversion) {
		_pn();
		_p("Calcium Project   " + _KernelVersion + "(" + _KV_rV_Text + ")");
		_p("RCapi Version :   " + _RCapi_Version);
		_p("Inside Version :   " + InsideVersion);
		_pn();
		_p("OpenCppLangTab Version :  " + $version_code_str + "  (" + $codename + ")");
		_p("OCLT Timestamp :   " + $buildtime);
		_p("Operating system :   " + _Run_SysKernel);
		_p("Run Command Argument :  " + native_argument);
		_p("File Path :  " + _$GetSelfFull);
		_p("Local Config :   " + buildshell);
		_pn();
		_p("Copyright FoxaXu");
		_p($year_message);
		_p("All rights reserved.");
		_pause();
		_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
		return 0;
	}

	_remove_sipcfg(Reg_Process_Map, Reg_Proces_runid);
	return 0;
}