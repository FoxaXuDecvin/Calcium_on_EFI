//Run Core

#pragma once
#include "../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS
//Define
std::string PartReadA(std::string Info, std::string StartMark, std::string EndMark, int RPartSizeA);

std::string araBuffer;
std::string $quomark = "\"";
std::string $sqmark = "\'";

//Run any program in any system
//    _system_autoRun("C:\Windows\Notepad.exe","Example.txt");
//    _system_autoRun("./home/desktop/app.run","example");
int resultCode;
int _system_autoRun(std::string file, std::string arguments) {
	if (!check_file_existenceA(file)) return -1001;

	if (_Run_SysKernel == Win32_kernel) {
		file = ReplaceChar(file, "/", "\\");
		araBuffer = $quomark + $quomark + file + $quomark + " " + arguments + $quomark;
		resultCode = system(araBuffer.c_str());
	}

	if (_Run_SysKernel == Linux_kernel) {
		file = ReplaceChar(file, "\\", "/");
		araBuffer = $sqmark + file + $sqmark + " " + arguments;
		resultCode = system(araBuffer.c_str());
	}

	// UEFI 无子进程能力。此前两个分支都不匹配时直接返回全局 resultCode
	// 的初值 0 —— 对调用方表现为"执行成功"，是假成功，比失败更糟。
	if (_Run_SysKernel == Efi_kernel) {
		resultCode = -1;
		return resultCode;
	}

	return resultCode;
}
std::string _sa_makeCache;
int _system_autoRun_A(std::string file, std::string arguments,bool SetFileLinuxChmod777) {
	if (!check_file_existenceA(file)) return -1001;

	if (_Run_SysKernel == Win32_kernel) {
		file = ReplaceChar(file, "/", "\\");
		araBuffer = $quomark + $quomark + file + $quomark + " " + arguments + $quomark;
		resultCode = system(araBuffer.c_str());
	}

	if (_Run_SysKernel == Linux_kernel) {
		file = ReplaceChar(file, "\\", "/");
		_sa_makeCache = "chmod 777 " + $sqmark + file + $sqmark;
		if (SetFileLinuxChmod777 == true)system(_sa_makeCache.c_str());
		araBuffer = $sqmark + file + $sqmark + " " + arguments;
		resultCode = system(araBuffer.c_str());
	}

	// UEFI 无子进程能力：诚实失败（同 _system_autoRun，防假成功）
	if (_Run_SysKernel == Efi_kernel) {
		resultCode = -1;
		return resultCode;
	}

	return resultCode;
}


int _system_autoRun_admin(std::string file, std::string arguments) {
	if (!check_file_existenceA(file)) return -1001;

	// UEFI 无提权/子进程能力：诚实失败（_Execute_Admin 桩恒 false，
	// 但 resultCode 全局初值 0 会被当成成功返回）
	if (_Run_SysKernel == Efi_kernel) {
		resultCode = -1;
		return resultCode;
	}

	_Execute_Admin(file, arguments);

	return resultCode;
}


std::string _get_argslonger(void) {
	return std::to_string(_argslonger);
}