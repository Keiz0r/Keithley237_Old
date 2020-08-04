#pragma once
#include "Keithley237.h"

class Script {
public:
	static void displayHelp();
	static void waitAndPrintProgress(int time);
	static void IV_Script1(Keithley237& keithley);
	static void pulse_Script1(Keithley237& keithley);
	static void playsong(Keithley237& keithley);
private:
	static void printProgress(float percentage);
};
