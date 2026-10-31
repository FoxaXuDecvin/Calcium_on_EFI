//Settings

#pragma once
#include "../../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS
//Settings
//Type :   true/false

//Display Launch Screen
bool __settings_displaylaunchscreen = true;
unsigned int __settings_displaylaunchscreen_time = 3;
// if true
// program will display launch screen
// On   Code/launchscreen.h

//Disabled throw Error
bool __settings_throwErrorMode = true;
//if true
//program will throw every error
// or false to disabled output all error info

//EFC Protocol File
std::string efc_Monitor = "empty.null.efc.socket";
bool _efc_settings$void_protocol_version_check = false;
//if true
// efc will not check the protocol version
// You may encounter the following issues
// File Connect API crash
// The file connection API is incompatible and doesn't prompt anything