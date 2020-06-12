#include "Keithley237.h"

ViStatus Keithley237::writeToInstrument(const char* msg) {
	//"X" stands for Execute command all the time
	//F0,0X - V/I source; DC/sweep mode (0/1)											page 3-27

	//D1,MESSAGEX = display message UPPERCASE ONLY!										page 3-25
	//D2,MESSAGEX - display message and save it in memory
	//D0X - return display to normal
	//Display has 18 characters
	//Any lowercase stands for SPACE

	//R1X - enable/Disable (0) triggers (can't set bias until trigger is active)
	//H0X - to activate trigger			(can't set bias until trigger is active)		page 3-35

	//BV,R,D - Voltage,Range, Delay. TO SET THE BIAS									page 3-19

	//N1X - Turn ON/OFF (0)

	//L1E-3,R - Current COMPLIANCE setting, R- range (1-1nA,2-10nA,4-1uA,7-1mA,9-100mA) page 3-38

	//A																					page 3-16

	//QN,ParamsX - create sweep list, N(0-fixed level, 1-linear, 2-log)(+3 = pulsed)	page 3-47
	//(+6 = Append sweep list)
	//Tested Q1,0,-2.2,0.1,3,100X

	//GI,F,L - Output format. I-items(0-8), F-format(0-4), L-lines(0-2)					page 3-28
	//Tested G4,2,2X

	//U List of errors																	page 3-62
	return status = viWrite(instr, (ViBuf)msg, (ViUInt32)strlen(msg), &writeCount);
}

void Keithley237::displayHelp() const {
	std::cout << "\nList of available functions:\n\nhelp - display help message\ntest2 - IV sweep (designed for big devices)\n\
testspecial - IV + stepped IV (designed for big devices)\ntestsmart\ntest0neg\ntest0pos\ntest0pulse\nit - I(t) measurement\n\
forming\nsdforming\nsdp\nsdppf\nivheat - IV sweep for oven (designed for small devices)\nexit - exit program\n" << std::endl;
}
