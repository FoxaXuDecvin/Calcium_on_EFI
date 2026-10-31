//Random Header

#pragma once
#include "../SourceHeader/include.h"
#define _CRT_SECURE_NO_WARNINGS

int _get_random(int min, int max) {
	using namespace std;
	string minb, maxb;
	minb = to_string(min);
	maxb = to_string(max);

	//string chars = "Min :  " + minb + "   Max :   " + maxb + "  Bug Report";
	//MessageBox(0, chars.c_str(), "MXBug Report", MB_OK);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(min, max); // Missing
	int outdata = dis(gen);

	//string dis_str = to_string(dis(gen));
	//MessageBox(0,dis_str.c_str(),"Bug check",MB_OK);

	return dis(gen);
}

std::string _get_random_s(int min, int max) {
	using namespace std;
	return to_string(_get_random(min, max));
}

