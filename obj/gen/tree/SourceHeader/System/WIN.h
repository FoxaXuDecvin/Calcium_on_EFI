#pragma once
#include<Windows.h> 
#include<direct.h>
#include <fstream>
#include <io.h>
#include <string>
#include <vector>
#include<ShlObj.h>

const std::string Win32_kernel = "Win32";
const std::string Linux_kernel = "Linux";
std::string _windows_userprofile = getenv("userprofile");
const std::string _Build_Path = _windows_userprofile + "/.OpenCppLangTab";

char* stringtochar;
char* stringToCharX(std::string tomessage) {

	const int len = tomessage.length();

	stringtochar = new char[len + 1];

	strcpy(stringtochar, tomessage.c_str());

	return stringtochar;
}

//Design For Windows
const std::string pathsign = "\\";
std::string RunPlatfom = "Windows (MSVC 64Bit)";//Must Include Windows/Linux one
std::string _Run_SysKernel = Win32_kernel;
//Linux s   Windows  ms
//Default Use s
void sleepapi(int num) {
	int cutth = num * 1000;
	Sleep(cutth);
	return;
}

void cleanConsole() {
	system("cls");
}

void foldercreateapi(std::string dir) {
	_mkdir(dir.c_str());
}

bool check_file_existenceA(const std::string& filename) {
	std::ifstream file(filename);
	if (file.is_open()) {
		return true;
	}
	else {
		return false;
	}
}
//NET CORE

//Use Windows API  <URLMON.DLL>
#include<urlmon.h>
#pragma comment(lib,"URlmon.lib")

LPCWSTR stringToLPCWSTR(std::string orig)
{
	using namespace std;
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t* wcstring = new wchar_t[sizeof(wchar_t) * (orig.length() - 1)];
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
	return wcstring;
}

//0-FAILED
//1-TRUE
bool URLDown(std::string URL, std::string Save) {
	using namespace std;
	remove(Save.c_str());

	LPCWSTR LcDsp = stringToLPCWSTR(Save);
	LPCWSTR LcURL = stringToLPCWSTR(URL);
	HRESULT applydownload = URLDownloadToFileW(
		nullptr,
		LcURL,
		LcDsp,
		0,
		nullptr
	);
	if (check_file_existenceA(Save)) {
		return true;
	}
	else {
		return false;
	}
	
}

//GetFilePath
// Resource File
std::string GetFilePath(void) {
	using namespace std;
	char szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
	/*
	strrchr:函数功能：查找一个字符c在另一个字符串str中末次出现的位置（也就是从str的右侧开始查找字符c首次出现的位置），
	并返回这个位置的地址。如果未能找到指定字符，那么函数将返回NULL。
	使用这个地址返回从最后一个字符c到str末尾的字符串。
	*/
	(strrchr(szFilePath, '\\'))[0] = 0; // 删除文件名，只获得路径字串//
	std::string path = szFilePath;
	return path;
}

std::string GetFileName(void) {
	using namespace std;
	char szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
	/*
	strrchr:函数功能：查找一个字符c在另一个字符串str中末次出现的位置（也就是从str的右侧开始查找字符c首次出现的位置），
	并返回这个位置的地址。如果未能找到指定字符，那么函数将返回NULL。
	使用这个地址返回从最后一个字符c到str末尾的字符串。
	*/
	std::string path = szFilePath;
	return path;
}

int removeDirectoryAPIX(std::string dir) {
	using namespace std;
	//在目录后面加上"\\*.*"进行第一次搜索
	std::string newDir = dir + "\\*.*";
	//用于查找的句柄
	intptr_t handle;
	struct _finddata_t fileinfo;
	//第一次查找
	handle = _findfirst(newDir.c_str(), &fileinfo);

	if (handle == -1) {
		//cout << "无文件" << endl;
		return 1;
	}

	do
	{
		if (fileinfo.attrib & _A_SUBDIR) {//如果为文件夹，加上文件夹路径，再次遍历
			if (strcmp(fileinfo.name, ".") == 0 || strcmp(fileinfo.name, "..") == 0)
				continue;

			// 在目录后面加上"\\"和搜索到的目录名进行下一次搜索
			newDir = dir + "\\" + fileinfo.name;
			removeDirectoryAPIX(newDir.c_str());//先遍历删除文件夹下的文件，再删除空的文件夹
			//cout << newDir.c_str() << endl;
			if (_rmdir(newDir.c_str()) == 0) {//删除空文件夹
				//cout << "delete empty dir success" << endl;
			}
			else {
				//cout << "delete empty dir error" << endl;
			}
		}
		else {
			std::string file_path = dir + "\\" + fileinfo.name;
			//cout << file_path.c_str() << endl;
			if (remove(file_path.c_str()) == 0) {//删除文件
				//cout << "delete file success" << endl;
			}
			else {
				//cout << "delete file error" << endl;
			}
		}
	} while (!_findnext(handle, &fileinfo));

	_findclose(handle);

	_rmdir(dir.c_str());
	return 1;
}

std::string Process_cache,Process_cache_dir;
void _fileapi_write(std::string _fa_file, std::string _fa_info);
std::string p1_filepath = GetFilePath();
void CreateFileMap_txt(std::string savefile, std::string dir) {
	using namespace std;
	//在目录后面加上"\\*.*"进行第一次搜索
	std::string newDir = dir + "\\*.*";
	//用于查找的句柄
	intptr_t handle;
	struct _finddata_t fileinfo;
	//第一次查找
	handle = _findfirst(newDir.c_str(), &fileinfo);

	if (handle == -1) {
		//cout << "无文件" << endl;
		return;
	}

	do
	{
		if (fileinfo.attrib & _A_SUBDIR) {//如果为文件夹，加上文件夹路径，再次遍历
			if (strcmp(fileinfo.name, ".") == 0 || strcmp(fileinfo.name, "..") == 0)
				continue;

			// 在目录后面加上"\\"和搜索到的目录名进行下一次搜索
			newDir = dir + "\\" + fileinfo.name;
			CreateFileMap_txt(savefile, newDir);//先遍历删除文件夹下的文件，再删除空的文件夹
		}
		else {
			std::string file_path = dir + "\\" + fileinfo.name;
			Process_cache = file_path;
			Process_cache = ReplaceChar(Process_cache, p1_filepath, "");
			//cout << file_path.c_str() << endl;
			_fileapi_write(savefile, ReplaceChar(Process_cache, "\\", "/"));
		}
	} while (!_findnext(handle, &fileinfo));

	_findclose(handle);

	return;
}

void CreateDirMap_txt(std::string savefile, std::string dir) {
	using namespace std;
	//在目录后面加上"\\*.*"进行第一次搜索
	std::string newDir = dir + "\\*.*";
	//用于查找的句柄
	intptr_t handle;
	struct _finddata_t fileinfo;
	//第一次查找
	handle = _findfirst(newDir.c_str(), &fileinfo);

	if (handle == -1) {
		//cout << "无文件" << endl;
		return;
	}

	do
	{
		if (fileinfo.attrib & _A_SUBDIR) {//如果为文件夹，加上文件夹路径，再次遍历
			if (strcmp(fileinfo.name, ".") == 0 || strcmp(fileinfo.name, "..") == 0)
				continue;

			// 在目录后面加上"\\"和搜索到的目录名进行下一次搜索
			newDir = dir + "\\" + fileinfo.name;
			//CreateFileMap_txt(savefile, newDir);//先遍历删除文件夹下的文件，再删除空的文件夹
			_fileapi_write(savefile, ReplaceChar(newDir, "\\", "/") + "/");
		}
		else {
			std::string file_path = dir + "\\" + fileinfo.name;
			Process_cache = file_path;
			//Process_cache = ReplaceChar(Process_cache, p1_filepath, "");
			//cout << file_path.c_str() << endl;
			_fileapi_write(savefile, ReplaceChar(Process_cache, "\\", "/"));
		}
	} while (!_findnext(handle, &fileinfo));

	_findclose(handle);

	return;
}


char *envbuffer;
std::string _SystemAPI_getenv(std::string Environment) {
	using namespace std;
	envbuffer = getenv(Environment.c_str());
	if (envbuffer == NULL) {
		return "Null.SystemAPI.getenv --> NullEnv";
	}
	return envbuffer;
}

bool _Execute_Admin(std::string File, std::string Args) {
	using namespace std;
	return ShellExecute(0, "runas", File.c_str(), Args.c_str(), 0, SW_SHOW);
}

void sleepapi_ms(int secondsNum) {
	using namespace std;
	Sleep(secondsNum);
	return;
}

void __sysbeep_api(int precodeA, int precodeB) {
	Beep(precodeA, precodeB);
	return;
}