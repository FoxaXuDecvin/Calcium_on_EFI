//Switch Tools
#pragma once
#include "../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS


//Message
std::string $_pause = "Press Enter...   ";

std::string _swt_buffer;

//Main
void _pause() {
	using namespace std;
	printf($_pause.c_str());
	getline(cin, _swt_buffer);
	return;
}

std::string _getline_type(void) {
	using namespace std;
	getline(cin, _swt_buffer);
	return _swt_buffer;
}