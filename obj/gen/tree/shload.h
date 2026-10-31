//Source Header Load
// Copyright FoxaXu 2024
// use #include"SourceHeader/shload.h"
//
//---

#define _CRT_SECURE_NO_WARNINGS

#pragma once

//Dedicated
#include"SourceHeader/include.h"
#include"SourceHeader/_sh_prt.h"
#include"SourceHeader/_sh_switch.h"

//Configuration
#include"SourceHeader/Settings/_sh_resource.h"
#include"SourceHeader/Settings/_sh_version.h"
#include"SourceHeader/Settings/sh_settings.h"

//Extend
#include"SourceHeader/_sh_random.h"
#include"SourceHeader/_sh_directory_api.h"
#include"SourceHeader/_sh_file_api.h"
#include"SourceHeader/_sh_openhttp.h"
#include"SourceHeader/_sh_runcore.h"
#include"SourceHeader/_sh_time.h"

//Engine
#include"Engine/_eg_sipcfg.h"
//#include"Engine/_eg_efc_api.h"

//SDK
#include"SDK/_sdk_textsuper.h"
#include"SDK/_sdk_textapi.h"
#include"SDK/_sdk_varspace.h"
//#include"SDK/_sdk_URLCodec.h" Not Support Linux


//END

//switchapi
bool argsSetExit = false;