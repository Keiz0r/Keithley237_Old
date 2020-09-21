#pragma once
#include "Keithley237.h"

#define LABEL_KSCRIPT_STATUS			"status"
#define LABEL_KSCRIPT_CONNECT			"connect"
#define LABEL_KSCRIPT_DISCONNECT		"disconnect"
#define LABEL_KSCRIPT_WRITETODEVICE		"write"
#define LABEL_KSCRIPT_DISPLAYHELP		"help"
#define LABEL_KSCRIPT_PULSE1			"pulse1"
#define LABEL_KSCRIPT_PULSE2			"pulse2"
#define LABEL_KSCRIPT_IT				"it"

class Script {
public:
	static void displayHelp();
	static void waitAndPrintProgress(int time);
	static void IV_Script1(Keithley237& keithley);
	static void pulse_Script1(Keithley237& keithley);
	static void pulse_Script2(Keithley237& keithley);
	static void It_Script(Keithley237& keithley);
	static void multipleLowVoltageIVs(Keithley237& keithley);
	static void playsong(Keithley237& keithley);
private:
	static void printProgress(float percentage);
};