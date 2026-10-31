
// Var Space  - Based on CalciumProject/VarSpace.h

#pragma once
#include"../shload.h"
#include <string>

#define _CRT_SECURE_NO_WARNINGS
//PUBLIC API

//HEAD
std::string _Old_VSAPI_TransVar(std::string Info);

//
std::string VarSpace = "varspace;";
int VarSpaceMax;

std::string readvar;
std::string varhead;
std::string varset;

std::string getfullLine;
std::string ResChar;
std::string ReplaceCharX;
std::string backinfo;

int checkCharA(std::string text, std::string chechchar) {
	using namespace std;
	const char* c = chechchar.c_str();

	if (text.find(c) != std::string::npos) {
		return 1;
	}
	else {
		return 0;
	}
}

std::string ReplaceCharA(std::string info, std::string replaceword, std::string nword) {
	using namespace std;
	//cout << "New Replace :  _" << info << "_  _" << replaceword << "_  _" << nword << "_" << endl;
	int checkanti = checkCharA(info, replaceword);
	if (checkanti == 1) {
		std::string dst_str = info;
		std::string::size_type pos = 0;
		while ((pos = dst_str.find(replaceword)) != std::string::npos)
		{
			dst_str.replace(pos, replaceword.length(), nword);
		}
		//cout << "Return Data :  _" << dst_str <<"_" << endl;
		return dst_str;
	}
	else {
		//cout << "Return Data :  _" << info << "_" << endl;
		return info;
	}
}

std::string cutlineblockB(
    const std::string& lines,
    const std::string& cutmark,
    int line)
{
    // line 使用 1-based 索引
    if (line <= 0 || lines.empty() || cutmark.empty()) {
        return "";
    }

    // 保持原函数行为：没有找到分隔符时返回 "NUL"
    if (lines.find_first_of(cutmark) == std::string::npos) {
        return "NUL";
    }

    std::size_t start = 0;
    int currentLine = 0;

    while (start < lines.size()) {
        // 跳过连续分隔符
        start = lines.find_first_not_of(cutmark, start);
        if (start == std::string::npos) {
            break;
        }

        const std::size_t end = lines.find_first_of(cutmark, start);
        ++currentLine;

        if (currentLine == line) {
            if (end == std::string::npos) {
                return lines.substr(start);
            }

            return lines.substr(start, end - start);
        }

        if (end == std::string::npos) {
            break;
        }

        start = end;
    }

    return "";
}

// 0 - OK
// 1 - Error
int _varspaceadd(std::string VarHead, std::string varinfo) {
	using namespace std;
	//cout << "GET " << endl;
	VarHead = ReplaceCharA(VarHead, " ", "");
	//cout << "START VAR" << endl;

	readbufferA = _Old_VSAPI_TransVar(VarHead);
	if (readbufferA == varinfo) {
		//cout << "NO EDIT" << endl;
		return 0;
	}

	if (varinfo == "") {
		//cout << "VAR EMPTY" << endl;
		_varspaceadd(VarHead, "{varspace.null}");
		return 0;
	}

	varinfo = ReplaceCharA(varinfo, " ", "%sbar%");
	varinfo = ReplaceCharA(varinfo, ";", "%selbar%");
	varinfo = ReplaceCharA(varinfo, "=","%eqbar%");
	varinfo = ReplaceCharA(varinfo, "$", "%mnbar%");
	varinfo = ReplaceCharA(varinfo, "\\", "%B_SLASH%");
	varinfo = ReplaceCharA(varinfo, "\n", "%$LineFeed%");
	if (checkCharA(VarHead, ";") == 1) {
		cout << "There is illegal text in the var" << endl;
		return 1;
	}

	if (VarSpaceMax == 0) {

		VarSpace = VarSpace + VarHead + "=" + varinfo + ";";

		VarSpaceMax++;

		return 0;
	}

	int startReadVar = 2;


BackFoundOldLine:
	//%var Mark Reader
	//cout << "readvar Head :  " << VarSpace << ".  " << VarHead << "_  " << to_std::string(startReadVar) << "_." << endl;
	readvar = cutlineblockB(VarSpace, ";", startReadVar);
	//cout << "readvar : " << readvar << endl;
	readvar = ReplaceCharA(readvar, ";", "");
	//cout << "readvar After Clean : " << readvar << endl;

	//Get XX = NN

	varhead = cutlineblockB(readvar, "=", 1);

	if (varhead == VarHead) {
		//cout << "Varhead : _" << varhead << "_. Select :  _" << VarHead << "_" << endl;
		varset = cutlineblockB(readvar, "=", 2);
		//cout << "Var Set :  _" << varset << "_" << endl;
		std::string delvars = varhead + "=" + varset + ";";
		std::string newvars = varhead + "=" + varinfo + ";";
		//cout << "Sort Delete Info :  _" << delvars << "_ .  ResData :  _" << VarSpace << "_" << endl;
		VarSpace = ReplaceCharA(VarSpace, delvars, newvars);
		//cout << "After CUT :  _" << VarSpace << "_" << endl;
		//cout << "After VarMAX :  _" << to_std::string(VarSpaceMax) << "_" << endl;
		//cout << "Var Space is Update.  Max : " << to_std::string(VarSpaceMax) << " . Message:   " << VarSpace << endl;
		return 0;
	}

	startReadVar--;
	//cout << "IF Command :  _" << to_std::string(startReadVar) << "_  . VMAX :  _" << to_std::string(VarSpaceMax) << "_" << endl;
	if (startReadVar == VarSpaceMax) {

		VarSpace = VarSpace + VarHead + "=" + varinfo + ";";

		VarSpaceMax++;

		return 0;
	}

	startReadVar++;
	startReadVar++;
	goto BackFoundOldLine;
}

void _varspacedelete(std::string VarHead) {
	using namespace std;
	//cout << "Delete Var Space :  _" << VarHead << "_" << endl;

	if (VarSpaceMax == 0) {
		return;
	}
	int startReadVar = 2;


BackFoundLine:
	//%var Mark Reader
	//cout << "readvar Head :  _" << VarSpace << "_.  _" << VarHead << "_  _" << to_std::string(startReadVar) << "_." << endl;
	readvar = cutlineblockB(VarSpace, ";", startReadVar);
	//cout << "readvar : " << readvar << endl;
	readvar = ReplaceCharA(readvar, ";", "");
	//cout << "readvar After Clean : " << readvar << endl;

	//Get XX = NN

	varhead = cutlineblockB(readvar, "=", 1);

	if (varhead == VarHead) {
		//cout << "Varhead : _" << varhead << "_. Select :  _" << VarHead << "_" << endl;
		varset = cutlineblockB(readvar, "=", 2);
		//cout << "Var Set :  _" << varset << "_" << endl;
		std::string delvars = varhead + "=" + varset + ";";
		//cout << "Sort Delete Info :  _" << delvars << "_ .  ResData :  _" << VarSpace << "_" << endl;
		VarSpace = ReplaceCharA(VarSpace, delvars, "");
		//cout << "After CUT :  _" << VarSpace << "_" << endl;
		VarSpaceMax--;
		//cout << "After VarMAX :  _" << to_std::string(VarSpaceMax) << "_" << endl;
		//cout << "Var Space is Update.  Max : " << to_std::string(VarSpaceMax) << " . Message:   " << VarSpace << endl;
		return;
	}

	startReadVar--;
	//cout << "IF Command :  _" << to_std::string(startReadVar) << "_  . VMAX :  _" << to_std::string(VarSpaceMax) << "_" << endl;
	if (startReadVar == VarSpaceMax) {
		return;
	}

	startReadVar++;
	startReadVar++;
	goto BackFoundLine;
}

std::string _Old_VSAPI_TransVar(std::string Info) {
	using namespace std;
	backinfo = Info;

	if (VarSpaceMax == 0) {
		return backinfo;
	}

	for (int startrollmax = 0, readmark = 2; startrollmax < VarSpaceMax; startrollmax++, readmark++) {
		getfullLine = cutlineblockB(VarSpace, ";", readmark);
		ResChar = cutlineblockB(getfullLine, "=", 1);
		ReplaceCharX = cutlineblockB(getfullLine, "=", 2);
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%sbar%", " ");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%selbar%", ";");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%eqbar%", "=");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%mnbar%", "$");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%B_SLASH%", "\\");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%$LineFeed%", "\n");
		//cout << "Process Env :  " << startrollmax << "      RM :  " << readmark << endl;
		int checkChar(std::string text, std::string chechchar);
		backinfo = ReplaceCharA(backinfo, ResChar, ReplaceCharX);
	}

	return backinfo;
}

std::string _getvarspace(std::string VarHead) {
	using namespace std;
	backinfo = VarHead;

	if (VarSpaceMax == 0) {
		return backinfo;
	}

	for (int startrollmax = 0, readmark = 2; startrollmax < VarSpaceMax; startrollmax++, readmark++) {
		getfullLine = cutlineblockB(VarSpace, ";", readmark);
		ResChar = cutlineblockB(getfullLine, "=", 1);
		ReplaceCharX = cutlineblockB(getfullLine, "=", 2);
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%sbar%", " ");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%selbar%", ";");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%eqbar%", "=");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%mnbar%", "$");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%B_SLASH%", "\\");
		ReplaceCharX = ReplaceCharA(ReplaceCharX, "%$LineFeed%", "\n");

		int checkChar(std::string text, std::string chechchar);
		backinfo = ReplaceCharA(backinfo, ResChar, ReplaceCharX);
	}

	return backinfo;
}

void _clear_varspace(void) {
	using namespace std;
	VarSpace = "varspace;";
	VarSpaceMax = 0;
}