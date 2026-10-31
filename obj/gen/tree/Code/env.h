//Environment Variable
//Env.H

#include"../SourceHeader/include.h"

//Here
std::string runscript = "{null}";
std::string script_args = "{null}";
std::string o_info = "{null}";

bool _setnextargs_runscript = false;
bool _setnextargs_addargs = false;
bool _setnextargs_addo = false;
bool _setproces_runid = false;
bool _setproces_perfid = false;
bool _debugMode = false;
bool _setproces_path = false;

//RUNMODE STRING

const std::string _runmode_null = "0000";
const std::string _runmode_runscript = "r1md";
const std::string _runmode_typehelp = "r2hp";
const std::string _runmode_autoexit = "r0cl";
const std::string _runmode_openshell = "r3se";
const std::string _runmode_listversion = "r4ve";
const std::string _runmode_optimi = "r5op";
const std::string _runmode_cstpmake = "r661";
const std::string _runmode_cstpunpack = "r662";
const std::string _runmode_helpargs = "r7ha";

const std::string _str_true = "true";
const std::string _str_false = "false";

std::string _runmode = _runmode_null;

bool _anticrash_services = false;

std::string InsideVersion = "C201";

std::string last_return;

//Here
bool _kernel_activate = false;
int _KV_relver$int;
bool _language_mode = false;

std::string kernelcmdVid;
std::string kernelenvVid;
std::string kernelSecureVid;
std::string gethookVid;
std::string isGetVid;
std::string sysexecVid;
std::string ThirdExecVid;

//RCapi

std::string _CTitle;
std::string _KV_rV_Text;
std::string _global_scriptload;
int _gf_cg = 0;
int _gf_cgmax = 1;
int _gf_line = 1;

std::string nt_sipcfg_open;

bool _setnextargs_loadenv;
bool _FastMode;

bool _CommandTracker;

bool _VarSpaceMFTracker;

bool _StackTracker;
std::string _StackTrackerFile = "$StackTracker.log";

std::string Reg_Proces_runid = "NoNameProcess";

std::string Reg_Process_Map;
std::string $coverscript, $coverscript_args;
std::string netwk_confirm;
int cc_script_privilege = 0;
int Network_AgentWaitTime = 20; //Seconds

bool AfterCompleteResest;