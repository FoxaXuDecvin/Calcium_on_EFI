//Print Advanced
#pragma once
#include "../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS


void _prtoutmsg(std::string message) {
	using namespace std;
	printf(message.c_str());
	printf("\n");
	return;
}

//Simple _prtoutmsg()
void _p(std::string message) {
	using namespace std;
	printf(message.c_str());
	printf("\n");
	return;
}

void _prts(std::string message) {
	using namespace std;
	printf(message.c_str());
	return;
}

void _prtendl(void) {
	using namespace std;
	printf("\n");
	return;
}

//Simple _prtend()
void _pn(void) {
	using namespace std;
	printf("\n");
	return;
}

void _coutmsg(std::string message) {
	using namespace std;
	cout << message << endl;
	return;
}