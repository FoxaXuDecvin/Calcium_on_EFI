//Based old Project https://github.com/FoxaXuDecvin/Calcium-Project
//Based on CASK.h
//Copyright FoxaXu 2024	

#pragma once
#include"../shload.h"
#define _CRT_SECURE_NO_WARNINGS
int maxread = 1;
std::string ReadNULLCheck;
int _textapi_getdocmax(std::string file) {
	using namespace std;
	if (!check_file_existence(file)) {
		cout << "Textapi.filemissing.getdocmax" << endl;
		return 0;
	}

	maxread = 1;

	while (true) {
		ReadNULLCheck = LineReader(file, maxread);
		if (ReadNULLCheck == "overline") {
			maxread--;
			return maxread;
		}
		maxread++;
	}

}

int maxdocsize;
void _textapi_typetext(std::string file) {
	using namespace std;
	if (check_file_existence(file)) {
		maxdocsize = getdocmax(file);

		maxdocsize++;

		for (int startRoll = 1; maxdocsize != startRoll; startRoll++) {
			cout << LineReader(file, startRoll) << endl;
		}
		return;
	}
	else {
		cout << "ERROR FILE NOT EXIST : " << file << endl;
		return;
	}
}