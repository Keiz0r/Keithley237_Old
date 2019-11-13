#pragma once
#include "visa.h"
#include <iostream>

class instrument {
public:
	ViStatus writeToInstrument(ViSession instrID, const char* msg);
private:
	ViStatus status;
	ViUInt32 writeCount;
	ViUInt32 retCount;
	ViSession instr;
	unsigned char buffer[10000];
	const char* Devicename = "GPIB0::2::INSTR";
};