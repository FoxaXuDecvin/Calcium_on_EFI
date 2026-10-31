//Source Header
// File API
// Copyright FoxaXu 2024

#include "../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS
//DEF
std::string _fileapi_textread(std::string File, int line_number);

//API
const std::string sw_os_NULL = "null";

std::string sw_openfile_str = sw_os_NULL;

int CountLines(std::string filename)
{
	using namespace std;
	ifstream ReadFile;
	int n = 0;
	std::string tmp;
	ReadFile.open(filename.c_str());
	if (ReadFile.fail())
	{
		return 0;
	}
	else
	{
		while (getline(ReadFile, tmp, '\n'))
		{
			n++;
		}
		ReadFile.close();
		return n;
	}
}
int getdocmax(std::string file) {
	int maxread = 1;
BackCheckRUN:
	std::string ReadNULLCheck = _fileapi_textread(file, maxread);
	if (ReadNULLCheck == "overline") {
		maxread--;
		return maxread;
	}
	maxread++;
	goto BackCheckRUN;
}


//Main
bool _existfile(const std::string& filename) {
	std::ifstream file(filename);
	if (file.is_open()) {
		return true;
	}
	else {
		return false;
	}
}

bool _fileapi_createmark(std::string File, std::string info) {
	std::ofstream CMark;
	CMark.open(File);
	CMark << info << std::endl;
	CMark.close();

	if (_existfile(File)) {
		return true;
	}
	else {
		return false;
	}
}

std::string _fileapi_textread(std::string File, int line_number) {
	if (_existfile(File)) {}
	else {
		_sh_throw_error("FileAPI.TextRead :  File Not Exist");
		return "FileNotExist";
	}
	int lines, i = 0;
	std::string temp;
	std::fstream file;
	file.open(File.c_str());
	lines = CountLines(File);

	if (line_number <= 0)
	{
		_sh_throw_error("FileAPI.TextRead :  Line Error On : " + File +" .  Line :  " + std::to_string(line_number));
		return "LineError";
	}
	if (file.fail())
	{
		_sh_throw_error("FileAPI.TextRead :  File Read Failed :  " + File);
		return "ReadFailed";
	}
	if (line_number > lines)
	{
		return "overline";
	}
	while (getline(file, temp) && i < line_number - 1)
	{
		i++;
	}
	file.close();
	return temp;
}

void _fileapi_typetext(std::string file) {
	if (_existfile(file)) {
		int maxdocsize = getdocmax(file);

		maxdocsize++;

		for (int startRoll = 1; maxdocsize != startRoll; startRoll++) {
			_api_prtmsg(_fileapi_textread(file, startRoll)+"\n");
		}
		return;
	}
	else {
		_sh_throw_error("FileAPI.TypeText :  File Not Exist");
		return;
	}
}

//File Write Not Clear
void _fileapi_write(std::string _fa_file, std::string _fa_info) {
	using namespace std;
	std::fstream f; 
	f.open(_fa_file, ios::out | ios::app); 
	f << _fa_info << std::endl;
	f.close();
	return;
}

//Copy/Delete
bool _fileapi_CpFile(std::string CopyFL, std::string CopyPath) {
	CopyFile(CopyFL.c_str(), CopyPath.c_str(), 0);
	if (check_file_existenceA(CopyPath)) {
		return true;
	}
	else {
		return false;
	}

	//Anti Cpp Code Check
	return false;
}

bool _fileapi_del(std::string DelFL) {
	remove(DelFL.c_str());

	if (check_file_existenceA(DelFL)) {
		return false;
	}
	else {
		return true;
	}

	//Anti Cpp Code Check
	return false;
}

//SoildWrite

bool _soildwrite_open(std::string sw_fileopen) {

	sw_openfile_str = sw_fileopen;

	return true;
}

bool _soildwrite_write(std::string info) {
	
	if (sw_openfile_str == sw_os_NULL) {
		return false;
	}

	_fileapi_write(sw_openfile_str,info);

	if (!check_file_existenceA(sw_openfile_str)) {
		_sh_throw_error("Failed open soildwrite :   " + sw_openfile_str);
		return false;
	}

	return true;
}

void _soildwrite_close() {
	sw_openfile_str = sw_os_NULL;

	return;
}