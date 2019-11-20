#pragma once
#include "visa.h"
#include <iostream>
#include <string>

class instrument {
public:
	ViStatus writeToInstrument(const char* msg);
	void readSweepFromInstrument(int place,int length);
	void readSweepFromInstrument();
private:
	ViStatus status;
	ViUInt32 writeCount;
	ViUInt32 retCount;
	ViSession instr;
	unsigned char buffer[10000];
	std::string Sbuffer;
	const char* Devicename = "GPIB0::2::INSTR";
};