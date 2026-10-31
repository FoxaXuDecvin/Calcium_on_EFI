//Cpp Includer
#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<stdio.h>
#include<random>
#include<thread>

#include"Settings/_sh_version.h"
#include"Settings/sh_settings.h"
#include"System/pub.h"

//Env H
#include"../Code/env.h"

#define _CRT_SECURE_NO_WARNINGS

; void _sh_throw_error(std::string message) {
	if (__settings_throwErrorMode == false)return;
	_api_prtmsg("----[ERROR](MESSAGE) :  " + message + "\n");
	return;
}


#if defined(__EFI__)
//UEFI
#include"System/EFI.h"

#elif defined(_WIN32)
//Windows
#include"System/WIN.h"

#else
//LINUX
#include"System/LUX.h"

#endif // _SystemAPI

//Load Core API

//Directory and File Full Path
std::string _$GetSelfPath = GetFilePath();
std::string _$GetSelfFull = GetFileName();

//END