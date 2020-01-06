#include <iostream>
#include "visa.h"
#include <bitset>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "Instrument.h"


#define MODE_DC		                   writeToDevice("F0,0X");
#define MODE_SWEEP	                   writeToDevice("F0,1X");
#define DATA_FORMAT_OUTPUT_TICK        writeToDevice("G15,0,0X");
#define DATA_FORMAT_OUTPUT_SWEEP       writeToDevice("G4,2,2X");
#define TRIGGER_ACTION		           writeToDevice("H0X");
#define OPERATE_OFF                    writeToDevice("N0X");
#define OPERATE_ON	                   writeToDevice("N1X");
#define FILTER_DISABLE		           writeToDevice("P0X");
#define FILTER_2READINGS	           writeToDevice("P1X");
#define FILTER_4READINGS	           writeToDevice("P2X");
#define FILTER_8READINGS	           writeToDevice("P3X");
#define FILTER_16READINGS	           writeToDevice("P4X");
#define FILTER_32READINGS	           writeToDevice("P5X");
#define TRIGGER_ENABLE                 writeToDevice("R1X");
#define TRIGGER_DISABLE                writeToDevice("R0X");
#define INTEGRATION_TIME_FAST          writeToDevice("S0X");	//  416 uSec |4-digit Resolution
#define INTEGRATION_TIME_MEDIUM        writeToDevice("S1X");	//    4 mSec |5-digit Resolution
#define INTEGRATION_TIME_LINECYCLE60HZ writeToDevice("S2X");	//16.67 mSec |5-digit Resolution
#define INTEGRATION_TIME_LINECYCLE50HZ writeToDevice("S3X");	//   20 mSec |5-digit Resolution
std::string userMessage;

static ViSession defaultRM, instr;
static ViStatus status;
static ViUInt32 writeCount;
static ViUInt32 retCount;
static unsigned char buffer[10000];	// buffer size for data coming from the device
int dataToRead;
int bytesToRead;
static std::string Sbuffer;
static const char* Devicename = "GPIB0::2::INSTR";


void deviceSettings() {
	status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 3000);	// Timeout value 3000 milliseconds (3sec)
	std::cout << "Device preset done" << std::endl;
	/*
	status = viFlush(instr,VI_IO_IN_BUF);
	status = viFlush(instr, VI_IO_OUT_BUF_DISCARD);
	status = viSetBuf(instr, VI_IO_OUT_BUF, 200);
	status = viSetBuf(instr, VI_IO_IN_BUF, 200);
	status = viSetBuf(instr, VI_READ_BUF, 200);
	status = viSetBuf(instr, VI_WRITE_BUF, 200);
	*/
}

void connectDevice() {
	status = viOpenDefaultRM(&defaultRM);
	if (status < VI_SUCCESS) {
		std::cout << "ERROR Initializing VISA driver" << std::endl;
	}
	status = viOpen(defaultRM, Devicename, VI_NULL, VI_NULL, &instr);
	std::cout << "Connecting to instrument: " << Devicename << std::endl;
	if (status < 0) {
		std::cout << "ERROR: Instrument not found" << std::endl;
	}
	else {
		deviceSettings();	// Apply preset
	}
}

void disconnectDevice() {
	status = viClose(instr);
	status = viClose(defaultRM);
	std::cout << "Disconnected instrument: " << Devicename << std::endl;
}

void printProgress(float percentage) {
	std::string str = "............................................................";
	int value = (int)(percentage * 100);
	int progressLength = (int)(percentage * str.length());
	std::cout << "\r" << std::setw(3) << value << "% [" << std::setw(60) << std::left << str.substr(0, progressLength) << "]";
	std::cout.flush();
}

void waitAndPrintProgress(int time) {
	for (int i = 0; i <= time; i++) {
		printProgress((float)i / (float)time);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << std::endl;
}

void displayStatus() {
	std::cout << "Status: " << status << std::endl;
	std::cout << "Binary Status "<< std::bitset<8>(status) << std::endl;
}

void writeToDevice(const char *msg) {
	status = viWrite(instr, (ViBuf)msg, (ViUInt32)strlen(msg), &writeCount);

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
}

void set_dc_bias(float bias,int range) {
	char Cmsg[15] = "B";
	char Cvalue[15];
	char Crange[4];
	sprintf_s(Cvalue, "%0.3f", bias);
	sprintf_s(Crange, "%i", range);
	strcat_s(Cmsg, Cvalue);
	strcat_s(Cmsg, ",");
	strcat_s(Cmsg, Crange);
	strcat_s(Cmsg, ",0X");	//wait time
	writeToDevice(Cmsg);
}

void setCurrentCompliance(const char* value, int range) {
	// Value as 5E-3
	char Cmsg[15] = "L";
	char Crange[4];
	sprintf_s(Crange, "%i", range);
	strcat_s(Cmsg, value);
	strcat_s(Cmsg, ",");
	strcat_s(Cmsg, Crange);
	strcat_s(Cmsg, "X");
	writeToDevice(Cmsg);
}

bool waitForSweepEnd() {
	status = viWrite(instr, (ViBuf)"U3X", (ViUInt32)strlen("U3X"), &writeCount);
	status = viRead(instr, buffer, 16, &retCount);	//reading how much data is available
	std::cout << "current masked stuff: " << buffer << std::endl;
	Sbuffer = reinterpret_cast<char const*>(buffer);
	Sbuffer.assign(Sbuffer, 16, 1);
	std::cout << "that number is : " << Sbuffer << std::endl;
	if (Sbuffer == "2"){
		std::cout << "FINALLY" << std::endl;
		return false;
	}
	else {
		std::cout << "NOT YET" << std::endl;
		return true;
	}
}

void read1FromDevice() {
	status = viWrite(instr, (ViBuf)"U11X", (ViUInt32)strlen("U11X"), &writeCount);
	status = viRead(instr, buffer, 100, &retCount);	//reading how much data is available
	Sbuffer = reinterpret_cast<char const*>(buffer);
	Sbuffer.assign(Sbuffer, 3, 4);
	dataToRead = std::stoi(Sbuffer);
	bytesToRead = dataToRead * 13;
	std::cout << "Data to read: " << dataToRead << std::endl;
	std::cout << "Bytes to read: " << bytesToRead << std::endl;

	status = viRead(instr, buffer, bytesToRead, &retCount);
	std::cout << "retCount: " << retCount << " bytes" << std::endl;
	std::cout << "Data read" << std::endl;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}
	Sbuffer.assign(Sbuffer, 0, 61*13);
	outputFileName << Sbuffer;
}

void read2FromDevice() {
	status = viWrite(instr, (ViBuf)"U11X", (ViUInt32)strlen("U11X"), &writeCount);
	status = viRead(instr, buffer, 100, &retCount);	//reading how much data is available
	Sbuffer = reinterpret_cast<char const*>(buffer);
	Sbuffer.assign(Sbuffer, 3, 4);
	dataToRead = std::stoi(Sbuffer);
	bytesToRead = dataToRead * 13;
	std::cout << "Data to read: " << dataToRead << std::endl;
	std::cout << "Bytes to read: " << bytesToRead << std::endl;

	status = viRead(instr, buffer, bytesToRead, &retCount);
	std::cout << "retCount: " << retCount << " bytes" << std::endl;
	std::cout << "Data read" << std::endl;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}
	Sbuffer.assign(Sbuffer, 0, 80 * 13);
	outputFileName << Sbuffer;
}

void read3FromDevice() {
	status = viWrite(instr, (ViBuf)"U11X", (ViUInt32)strlen("U11X"), &writeCount);
	status = viRead(instr, buffer, 100, &retCount);	//reading how much data is available
	Sbuffer = reinterpret_cast<char const*>(buffer);
	Sbuffer.assign(Sbuffer, 3, 4);
	dataToRead = std::stoi(Sbuffer);
	bytesToRead = dataToRead * 13;
	std::cout << "Data to read: " << dataToRead << std::endl;
	std::cout << "Bytes to read: " << bytesToRead << std::endl;

	status = viRead(instr, buffer, bytesToRead, &retCount);
	std::cout << "retCount: " << retCount << " bytes" << std::endl;
	std::cout << "Data read" << std::endl;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}
	Sbuffer.erase(Sbuffer.size() - 4*13);
	outputFileName << Sbuffer;
}

bool readSmartFromDevice(int data, bool wait, float wait_multiplier, bool do_analysis_of_RonRoff) {
	if (wait) {
		int time = (int)std::ceil(data * wait_multiplier);
		waitAndPrintProgress(time);
	}
	status = viRead(instr, buffer, 10000, &retCount);
	std::cout << "Read: " << (retCount - 1) / 13 << " data points;\t" << retCount << " bytes" << std::endl;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}

	Sbuffer.assign(Sbuffer, 0, data * 13);
	outputFileName << Sbuffer;

	if (do_analysis_of_RonRoff) {
		std::string::size_type sz;
		float Ron = std::stof(Sbuffer.substr(2 * 13, 12), &sz);
		float Roff = std::stof(Sbuffer.substr(43 * 13, 12), &sz);
		std::cout << "\nON current = " << Ron *  pow(10, 6) << " uA" << std::endl;
		std::cout << "OFF current= "   << Roff * pow(10, 6) << " uA" << std::endl;
		std::cout << "Ron/Roff ratio= " << Ron / Roff << std::endl;
		if ((Ron / Roff) < 3.0f) {
			std::cout << "!!! Alert, failure encountered !!!  Ron/Roff < 3" << std::endl;
			return false;
		}
	}
	return true;
}

void readMessageFromDevice() {
	status = viRead(instr, buffer, 100, &retCount);
	std::cout << "Message: " << buffer << std::endl;
}

void IV_meas() {
	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	char str1[20] = "L";
	char str2[4] = ",0X";
	strcat_s(str1, currentCompliance);
	strcat_s(str1, str2);
	std::cout << str1 << std::endl;
	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	bool make_prep = false;
	std::cout << "Do prep? 1/0\n";
	std::cin >> make_prep;
	status = viWrite(instr, (ViBuf)"L4E-4,0X", (ViUInt32)strlen("L4E-4,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0,0,0X", (ViUInt32)strlen("B0,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"F0,1X", (ViUInt32)strlen("F0,1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"M2,X", (ViUInt32)strlen("M2,X"), &writeCount);	//Mask to get end of sweep
	status = viWrite(instr, (ViBuf)"G4,2,2X", (ViUInt32)strlen("G4,2,2X"), &writeCount);	//Data Format IS IMPORTANT!!!

	if (make_prep) {
		status = viWrite(instr, (ViBuf)"L1E-1,0X", (ViUInt32)strlen("L1E-1,0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q1,0.1,4,0.1,0,400X", (ViUInt32)strlen("Q1,0.1,4,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,3.9,0,0.1,0,400X", (ViUInt32)strlen("Q7,0,3.9,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(40));
	}

	for (int i = 0; i < amountOfRuns; ++i) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		status = viWrite(instr, (ViBuf)str1, (ViUInt32)strlen(str1), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q1,0,-3,0.1,0,400X", (ViUInt32)strlen("Q1,0,-3,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,-2.9,0,0.1,0,400X", (ViUInt32)strlen("Q7,0,-2.9,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(50));
		status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
		read1FromDevice();
		status = viWrite(instr, (ViBuf)"L1E-1,0X", (ViUInt32)strlen("L1E-1,0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q1,0.1,4,0.1,0,400X", (ViUInt32)strlen("Q1,0.1,3,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,3.9,0,0.1,0,400X", (ViUInt32)strlen("Q7,0,2.9,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(45));
		status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
		read2FromDevice();
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void IV_meas2() {
	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	char str1[20] = "L";
	char str2[4] = ",0X";
	strcat_s(str1, currentCompliance);
	strcat_s(str1, str2);
	std::cout << str1 << std::endl;
	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	bool make_prep = false;
	std::cout << "Do prep? 1/0\n";
	std::cin >> make_prep;
	status = viWrite(instr, (ViBuf)"L4E-4,0X", (ViUInt32)strlen("L4E-4,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0,0,0X", (ViUInt32)strlen("B0,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"F0,1X", (ViUInt32)strlen("F0,1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"M2,X", (ViUInt32)strlen("M2,X"), &writeCount);	//Mask to get end of sweep
	status = viWrite(instr, (ViBuf)"G4,2,2X", (ViUInt32)strlen("G4,2,2X"), &writeCount);	//Data Format IS IMPORTANT!!!
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);

	if (make_prep) {
		status = viWrite(instr, (ViBuf)"L1E-1,0X", (ViUInt32)strlen("L1E-1,0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q1,0.1,4,0.1,0,400X", (ViUInt32)strlen("Q1,0.1,4,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,3.9,0,0.1,0,400X", (ViUInt32)strlen("Q7,0,3.9,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(40));
	}

	for (int i = 0; i < amountOfRuns; ++i) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		status = viWrite(instr, (ViBuf)str1, (ViUInt32)strlen(str1), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q1,0,-3,0.1,0,400X", (ViUInt32)strlen("Q1,0,-3,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,-2.9,0,1,0,400X", (ViUInt32)strlen("Q7,0,-2.9,1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
	//	std::this_thread::sleep_for(std::chrono::seconds(29));
		readSmartFromDevice(-3 * -10 + 1 + 4, true, 0.6f, false);	// (-3 * -20 + 1)
		status = viWrite(instr, (ViBuf)"L1E-1,0X", (ViUInt32)strlen("L1E-1,0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q1,0.1,4,0.1,0,400X", (ViUInt32)strlen("Q1,0.1,3,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,3.9,0.3,1.5,0,400X", (ViUInt32)strlen("Q7,3.9,0.3,1.5,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
	//	std::this_thread::sleep_for(std::chrono::seconds(36));
		readSmartFromDevice(4 * 10 + 4, true, 0.6f, false);	// (4 * 20)
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

//TODO it
void setupSmart() {
	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	char str1[30] = "L";
	strcat_s(str1, currentCompliance);
	strcat_s(str1, ",0X");
	std::cout << str1 << std::endl;

	std::cout << "Negative up to? (default -3) \n";
	char negative_limit[5];
	std::cin >> negative_limit;
	double Fnegative_limit = strtof(negative_limit, nullptr);
	strcat_s(negative_limit, ",");
	char str2[30] = "Q1,0,";
	strcat_s(str2, negative_limit);

	std::cout << "Voltage step? (default 0.1) \n";
	char voltage_step[20];
	std::cin >> voltage_step;
	strcat_s(str2, voltage_step);
	strcat_s(str2, ",0,");
	double Fvoltage_Step = strtof(voltage_step, nullptr);

	std::cout << "TimeStep? (default 400) \n";
	char timeStep[10];
	std::cin >> timeStep;
	strcat_s(str2, timeStep);
	strcat_s(str2, "X");
	std::cout << str2 << std::endl;

	std::cout << "Positive up to? (default 4) \n";
	char positive_limit[5];
	std::cin >> positive_limit;
	double Fpositive_limit = strtof(positive_limit, nullptr);
	strcat_s(positive_limit, ",");
	char str4[30] = "Q1,";
	strcat_s(str4, voltage_step);
	strcat_s(str4, ",");
	strcat_s(str4, positive_limit);
	strcat_s(str4, voltage_step);
	strcat_s(str4, ",0,");
	strcat_s(str4, timeStep);
	strcat_s(str4, "X");
	std::cout << str4 << std::endl;

	char str3[30] = "Q7,";
	char tt[10];
	sprintf_s(tt, "%0.1f", Fnegative_limit + Fvoltage_Step);
	strcat_s(str3, tt);
	strcat_s(str3, ",0,1,0,");
	strcat_s(str3, timeStep);
	strcat_s(str3, "X");
	std::cout << str3 << std::endl;

	char str5[30] = "Q7,";
	sprintf_s(tt, "%0.1f", Fpositive_limit - Fvoltage_Step);
	strcat_s(str5, tt);
	strcat_s(str5, ",0.3,1.5,0,");
	strcat_s(str5, timeStep);
	strcat_s(str5, "X");
	std::cout << str5 << std::endl;
}

void IV_measSmart() {
	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;

	std::cout << "Negative up to? (default -3) \n";
	char negative_limit[5];
	std::cin >> negative_limit;
	float Fnegative_limit = strtof(negative_limit, nullptr);
	strcat_s(negative_limit, ",");
	char str2[30] = "Q1,0,";
	strcat_s(str2, negative_limit);

	std::cout << "Voltage step? (default 0.1) \n";
	char voltage_step[20];
	std::cin >> voltage_step;
	strcat_s(str2, voltage_step);
	strcat_s(str2, ",0,");
	float Fvoltage_Step = strtof(voltage_step, nullptr);

	std::cout << "TimeStep? (default 400) \n";
	char timeStep[10];
	std::cin >> timeStep;
	float FtimeStep = strtof(timeStep, nullptr);
	strcat_s(str2, timeStep);
	strcat_s(str2, "X");

	std::cout << "Positive up to? (default 4) \n";
	char positive_limit[5];
	std::cin >> positive_limit;
	float Fpositive_limit = strtof(positive_limit, nullptr);
	strcat_s(positive_limit, ",");
	char str4[30] = "Q1,";
	strcat_s(str4, voltage_step);
	strcat_s(str4, ",");
	strcat_s(str4, positive_limit);
	strcat_s(str4, voltage_step);
	strcat_s(str4, ",0,");
	strcat_s(str4, timeStep);
	strcat_s(str4, "X");

	char str3[30] = "Q7,";
	char tt[10];
	sprintf_s(tt, "%0.3f", Fnegative_limit + Fvoltage_Step);
	strcat_s(str3, tt);
	strcat_s(str3, ",0,1,0,");
	strcat_s(str3, timeStep);
	strcat_s(str3, "X");

	char str5[30] = "Q7,";
	sprintf_s(tt, "%0.3f", Fpositive_limit - Fvoltage_Step);
	strcat_s(str5, tt);
	strcat_s(str5, ",0.3,1.5,0,");
	strcat_s(str5, timeStep);
	strcat_s(str5, "X");

	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	INTEGRATION_TIME_FAST
	FILTER_DISABLE
	OPERATE_OFF
	TRIGGER_DISABLE
	set_dc_bias(0.0f,0);
	MODE_SWEEP
//	status = viWrite(instr, (ViBuf)"M2,X", (ViUInt32)strlen("M2,X"), &writeCount);	//Mask to get end of sweep
	DATA_FORMAT_OUTPUT_SWEEP
	TRIGGER_ENABLE
	OPERATE_ON
	
	std::ofstream outputFailuresFileName("KeithOUTFailures.txt", std::ios::app);
	for (int i = 0; i < amountOfRuns; ++i) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		setCurrentCompliance(currentCompliance, 0);
		writeToDevice(str2);
		writeToDevice(str3);
		TRIGGER_ACTION
		readSmartFromDevice(std::ceil(Fnegative_limit / Fvoltage_Step) * -1 + 1 + 4, true, 0.0015f * FtimeStep, false);	// (-3 * -20 + 1)
		setCurrentCompliance("1E-1", 0);
		writeToDevice(str4);
		writeToDevice(str5);
		TRIGGER_ACTION
		if (!readSmartFromDevice(std::ceil(Fpositive_limit / Fvoltage_Step) + 4, true, 0.0012f * FtimeStep, true)) {

			outputFailuresFileName << i << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(10));
			std::cout << " - - - - - - Dear lord please save us - - - - - - " << std::endl;
			setCurrentCompliance(currentCompliance, 0);
			status = viWrite(instr, (ViBuf)"Q1,0,-3,0.1,0,400X", (ViUInt32)strlen("Q1,0,-3,0.1,0,400X"), &writeCount);
			status = viWrite(instr, (ViBuf)"Q7,-2.9,0,1,0,400X", (ViUInt32)strlen("Q7,0,-2.9,1,0,400X"), &writeCount);
			TRIGGER_ACTION
		//	readSmartFromDevice(-3 * -10 + 1 + 4, true, 0.6f, false);	// (-3 * -20 + 1)
			std::this_thread::sleep_for(std::chrono::seconds(70));
			setCurrentCompliance("1E-1", 0);
			status = viWrite(instr, (ViBuf)"Q1,0.1,8,0.1,0,400X", (ViUInt32)strlen("Q1,0.1,8,0.1,0,400X"), &writeCount);
			status = viWrite(instr, (ViBuf)"Q7,7.9,0.3,1.5,0,400X", (ViUInt32)strlen("Q7,7.9,0.3,1.5,0,400X"), &writeCount);
			TRIGGER_ACTION
		//	readSmartFromDevice(4 * 10 + 4, true, 0.6f, false);	// (4 * 20)
			std::this_thread::sleep_for(std::chrono::seconds(70));

		}
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void IV_meas_thermal_Smart() {
	/*
	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	*/
	setCurrentCompliance("1E-7", 3);

	std::cout << "Negative up to? (default -0.3) \n";
	char negative_limit[10];
	std::cin >> negative_limit;
	float Fnegative_limit = strtof(negative_limit, nullptr);
	strcat_s(negative_limit, ",");
	char str2[30] = "Q1,0,";
	strcat_s(str2, negative_limit);

	std::cout << "Voltage step? (default 0.01) \n";
	char voltage_step[20];
	std::cin >> voltage_step;
	strcat_s(str2, voltage_step);
	strcat_s(str2, ",0,");
	float Fvoltage_Step = strtof(voltage_step, nullptr);

	std::cout << "TimeStep? (default 400) \n";
	char timeStep[10];
	std::cin >> timeStep;
	float FtimeStep = strtof(timeStep, nullptr);
	strcat_s(str2, timeStep);
	strcat_s(str2, "X");

	std::cout << "Positive up to? (default 0.3) \n";
	char positive_limit[10];
	std::cin >> positive_limit;
	float Fpositive_limit = strtof(positive_limit, nullptr);
	strcat_s(positive_limit, ",");
	char str4[30] = "Q1,";
	strcat_s(str4, voltage_step);
	strcat_s(str4, ",");
	strcat_s(str4, positive_limit);
	strcat_s(str4, voltage_step);
	strcat_s(str4, ",0,");
	strcat_s(str4, timeStep);
	strcat_s(str4, "X");

	char str3[30] = "Q7,";
	char tt[10];
	sprintf_s(tt, "%0.3f", Fnegative_limit + Fvoltage_Step);
	strcat_s(str3, tt);
	strcat_s(str3, ",0,");
	strcat_s(str3, voltage_step);
	strcat_s(str3, ",0,");
	strcat_s(str3, timeStep);
	strcat_s(str3, "X");

	char str5[30] = "Q7,";
	sprintf_s(tt, "%0.3f", Fpositive_limit - Fvoltage_Step);
	strcat_s(str5, tt);
	strcat_s(str5, ",0,");
	strcat_s(str5, voltage_step);
	strcat_s(str5, ",0,");
	strcat_s(str5, timeStep);
	strcat_s(str5, "X");

	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	INTEGRATION_TIME_LINECYCLE60HZ
	FILTER_8READINGS
	OPERATE_OFF
	TRIGGER_DISABLE
	set_dc_bias(0.0f, 0);
	MODE_SWEEP
	//	status = viWrite(instr, (ViBuf)"M2,X", (ViUInt32)strlen("M2,X"), &writeCount);	//Mask to get end of sweep
	DATA_FORMAT_OUTPUT_SWEEP
	TRIGGER_ENABLE
	OPERATE_ON

	std::ofstream outputFailuresFileName("KeithOUTFailures.txt", std::ios::app);
	for (int i = 0; i < amountOfRuns; ++i) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		writeToDevice(str2);
		writeToDevice(str3);
		TRIGGER_ACTION
		readSmartFromDevice((Fnegative_limit / Fvoltage_Step) * -2 + 1, true, 0.0017f * FtimeStep, false);	// (-3 * -20 + 1)
		writeToDevice(str4);
		writeToDevice(str5);
		TRIGGER_ACTION
		readSmartFromDevice((Fpositive_limit / Fvoltage_Step) * 2, true, 0.0017f * FtimeStep, false);
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void Itmeas() {
	std::cout << "I(t) measurement mode\n";
	std::cout << "Voltage?\n";
	char ItVoltage[5];
	std::cin >> ItVoltage;
	char str1[20] = "B";
	char str2[6] = ",0,0X";
	strcat_s(str1, ItVoltage);
	strcat_s(str1, str2);
	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	char str3[20] = "L";
	char str4[4] = ",0X";
	strcat_s(str3, currentCompliance);
	strcat_s(str3, str4);
	std::ofstream outputFileName("KeithIt.txt", std::ios::app);
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"F0,0X", (ViUInt32)strlen("F0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"G15,0,0X", (ViUInt32)strlen("G15,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)str1, (ViUInt32)strlen(str1), &writeCount);
	status = viWrite(instr, (ViBuf)str3, (ViUInt32)strlen(str3), &writeCount);	//CC Settings
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	while (1) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		float measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0,36);//remove 1st letters
		outputFileName << measTime << "\t"<< Sbuffer << std::endl;
	}
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
}

void forming() {
	std::cout << "Forming mode\n";
	bool next_step = false;
	float measTime;
	float target;
	std::string::size_type sz;
	std::ofstream outputFileName("Forming.txt", std::ios::app);
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"F0,0X", (ViUInt32)strlen("F0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"G15,0,0X", (ViUInt32)strlen("G15,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0.3,0,0X", (ViUInt32)strlen("B0.3,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"L5E-3,0X", (ViUInt32)strlen("L5E-3,0X"), &writeCount);	//CC Settings
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	//show initial state
	for (int i = 0; i <= 15; ++i) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
	}
	status = viWrite(instr, (ViBuf)"B3,0,0X", (ViUInt32)strlen("B3,0,0X"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	while (!next_step) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
		target = std::stof(Sbuffer, &sz);
		if (target >= 0.0008f) {
			next_step = true;
			std::cout << "Step 1 complete!\n";
		}
	}
	next_step = false;
	status = viWrite(instr, (ViBuf)"B5,0,0X", (ViUInt32)strlen("B5,0,0X"), &writeCount);
	while (!next_step) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
		target = std::stof(Sbuffer, &sz);
		if (target >= 0.0049f) {
			next_step = true;
			std::cout << "Step 2 complete!\n";
		}
	}
	next_step = false;
	status = viWrite(instr, (ViBuf)"B-3,0,0X", (ViUInt32)strlen("B-2,0,0X"), &writeCount);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	while (!next_step) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
		target = std::stof(Sbuffer, &sz);
		if (target >= 0.003f) {
			next_step = true;
			std::cout << "Negative step complete!\n";
		}
	}
	//show formed state
	status = viWrite(instr, (ViBuf)"B0.3,0,0X", (ViUInt32)strlen("B0.3,0,0X"), &writeCount);
	for (int i = 0; i <= 25; ++i) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
	}
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	std::cout << "Forming complete!\n";
}

void disco() {
	status = viWrite(instr, (ViBuf)"D1,NUJNOnBOLSHEX", (ViUInt32)strlen("D1,NUJNOnBOLSHEX"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	status = viWrite(instr, (ViBuf)"D1,DENEGX", (ViUInt32)strlen("D1,DENEGX"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	status = viWrite(instr, (ViBuf)"D1,DENEGnPROSTOnTAKX", (ViUInt32)strlen("D1,DENEGnPROSTOnTAKX"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	status = viWrite(instr, (ViBuf)"D1,PUSTnZAnNASX", (ViUInt32)strlen("D1,PUSTnZAnNASX"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	status = viWrite(instr, (ViBuf)"D1,RABOTAETX", (ViUInt32)strlen("D1,RABOTAETX"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	status = viWrite(instr, (ViBuf)"D1,TALANTLIVIYX", (ViUInt32)strlen("D1,TALANTLIVIYX"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	status = viWrite(instr, (ViBuf)"D1,INDUSX", (ViUInt32)strlen("D1,INDUSX"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	status = viWrite(instr, (ViBuf)"D0X", (ViUInt32)strlen("D0X"), &writeCount);
	std::this_thread::sleep_for(std::chrono::seconds(5));
}

void small_device_pulsed_mode() {
	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	bool enable_healing;
	while (1) {
		std::cout << "Enable healing? 1/0\n";
		std::cin >> enable_healing;
		if (enable_healing) {
			std::cout << "Healing enabled\n";
			break;
		}
		else if (!enable_healing) {
			std::cout << "Healing disabled\n";
			break;
		}
		std::cout << "Wrong input\n";
	}
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	std::ofstream readsFileName("reads.txt", std::ios::app);
	std::ofstream assist_forming_counter_file("afc.txt", std::ios::app);
	float failTestNum;
	std::string temp1;
	std::string temp2;
	std::string temp3;
	std::string::size_type sz;
	bool get_out = false;
	int set_fail_counter = 0;
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0,0,0X", (ViUInt32)strlen("B0,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"L0.1E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);	//CC Settings; not necessary here, but the instrument doesn't argue.
	status = viWrite(instr, (ViBuf)"F0,1X", (ViUInt32)strlen("F0,1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"G4,2,2X", (ViUInt32)strlen("G4,2,2X"), &writeCount);
	status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);	//create sweep
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	for (int i = 0; i < amountOfRuns; ++i) {
		status = viWrite(instr, (ViBuf)"L0.1E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q3,-5,2,1,500,500X", (ViUInt32)strlen("Q9,-4,2,1,500,500X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q9,0.3,2,1,500,500X", (ViUInt32)strlen("Q9,0.3,2,1,500,500X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		//read1
		status = viWrite(instr, (ViBuf)"U11X", (ViUInt32)strlen("U11X"), &writeCount);
		status = viRead(instr, buffer, 100, &retCount);	
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.assign(Sbuffer, 3, 4);
		dataToRead = std::stoi(Sbuffer);
		bytesToRead = dataToRead * 13;
		//std::cout << "Data to read: " << dataToRead << std::endl;
		//std::cout << "Bytes to read: " << bytesToRead << std::endl;

		status = viRead(instr, buffer, bytesToRead, &retCount);
		//std::cout << "retCount: " << retCount << " bytes" << std::endl;
		//std::cout << "Data read" << std::endl;
		Sbuffer = reinterpret_cast<char const*>(buffer);
		//Format data into column
		int position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer,0,52);	//TODO instead of 52 refer to dataToRead
	//	std::cout << Sbuffer << std::endl;
		outputFileName << Sbuffer;
		temp3.assign(Sbuffer, 26, 12);

		//FAILCHECK
		temp1.assign(Sbuffer, 27, 7);
		temp2.assign(Sbuffer, 36, 2);
	//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
		failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
	//	std::cout << "Read1 current: " << failTestNum << " Amps\n";	//for testing. check Sbuffers arrigns of temp1 and temp2 to debug;
		if (failTestNum <= 0.00000099f) {	//Less than 1uA
			while (!get_out) {
				set_fail_counter++;
				std::cout << "Read1 threshold failed\n";
				status = viWrite(instr, (ViBuf)"Q3,5,2,1,1500,200X", (ViUInt32)strlen("Q3,5,2,1,2500,200X"), &writeCount);	//create sweep
				status = viWrite(instr, (ViBuf)"Q9,-5,2,1,500,500X", (ViUInt32)strlen("Q9,-5,2,1,500,500X"), &writeCount);
				status = viWrite(instr, (ViBuf)"Q9,0.3,2,1,500,500X", (ViUInt32)strlen("Q9,0.3,2,1,500,500X"), &writeCount);
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::cout << "- - - - - - - Applied +5V- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(4));
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				//read3
				status = viWrite(instr, (ViBuf)"U11X", (ViUInt32)strlen("U11X"), &writeCount);
				status = viRead(instr, buffer, 100, &retCount);	//reading how much data is available
				Sbuffer = reinterpret_cast<char const*>(buffer);
				Sbuffer.assign(Sbuffer, 3, 4);
				dataToRead = std::stoi(Sbuffer);
				bytesToRead = dataToRead * 13;
			//	std::cout << "Data to read: " << dataToRead << std::endl;
			//	std::cout << "Bytes to read: " << bytesToRead << std::endl;

				status = viRead(instr, buffer, bytesToRead, &retCount);
			//	std::cout << "retCount: " << retCount << " bytes" << std::endl;
			//	std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 78);
				outputFileName << Sbuffer;
				assist_forming_counter_file << i + 1 << " ";
			//	std::cout << Sbuffer << std::endl;

				temp1.assign(Sbuffer, 53, 7);
				temp2.assign(Sbuffer, 62, 2);
				failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
			//	std::cout << "Read1 current after +5V help: " << failTestNum << " Amps\n";	//for testing. check Sbuffers arrigns of temp1 and temp2 to debug;
				if (failTestNum >= 0.00000099f) {
					get_out = true;
					temp3.assign(Sbuffer, 52, 12);
					std::cout << "Read1 threshold passed with help\n";
				}
				if (set_fail_counter >= 30 && enable_healing) {	//HEALING EXPERIMENTAL
					std::cout << "Set Failed 30 times\nPerforming healing\n";
					while (1){
						status = viWrite(instr, (ViBuf)"L8E-4,7X", (ViUInt32)strlen("L8E-4,7X"), &writeCount);
						status = viWrite(instr, (ViBuf)"Q3,5,2,1,1000,1000X", (ViUInt32)strlen("Q3,5,2,1,1000,1000X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(2));
						std::this_thread::sleep_for(std::chrono::milliseconds(300));
						status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);
						status = viWrite(instr, (ViBuf)"Q3,5,2,1,300,500X", (ViUInt32)strlen("Q3,5,2,1,300,500X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(1));
						status = viWrite(instr, (ViBuf)"L0.1E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);
						status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(1));
						std::this_thread::sleep_for(std::chrono::milliseconds(200));
						status = viRead(instr, buffer, bytesToRead, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);

						temp3.assign(Sbuffer, 0, 12);
						temp1.assign(Sbuffer, 1, 7);
						temp2.assign(Sbuffer, 10, 2);
						//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
						failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
						//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

						if (failTestNum <= 0.00000019f) {
							while (!get_out) {
								status = viWrite(instr, (ViBuf)"Q3,5,2,1,1500,200X", (ViUInt32)strlen("Q3,5,2,1,2500,200X"), &writeCount);	//create sweep
								status = viWrite(instr, (ViBuf)"Q9,-5,2,1,500,500X", (ViUInt32)strlen("Q9,-5,2,1,500,500X"), &writeCount);
								status = viWrite(instr, (ViBuf)"Q9,0.3,2,1,500,500X", (ViUInt32)strlen("Q9,0.3,2,1,500,500X"), &writeCount);
								status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
								std::this_thread::sleep_for(std::chrono::seconds(4));
								std::this_thread::sleep_for(std::chrono::milliseconds(500));
								status = viRead(instr, buffer, bytesToRead, &retCount);
								//	std::cout << "retCount: " << retCount << " bytes" << std::endl;
								//	std::cout << "Data read" << std::endl;
								Sbuffer = reinterpret_cast<char const*>(buffer);
								//Format data into column
								position = Sbuffer.find(",");
								while (position != std::string::npos) {
									Sbuffer.replace(position, 1, "\n");
									position = Sbuffer.find(",", position + 1);
								}
								Sbuffer.assign(Sbuffer, 0, 78);
								outputFileName << Sbuffer;
								assist_forming_counter_file << i + 1 << "H ";
								//	std::cout << Sbuffer << std::endl;

								temp1.assign(Sbuffer, 53, 7);
								temp2.assign(Sbuffer, 62, 2);
								failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
								//	std::cout << "Read1 current after +5V help: " << failTestNum << " Amps\n";	//for testing. check Sbuffers arrigns of temp1 and temp2 to debug;
								if (failTestNum >= 0.00000099f) {
									get_out = true;
									temp3.assign(Sbuffer, 52, 12);
								}
							}
							std::cout << "Healing procedure performed\n";
							break;
						}
					}
				}
			}
			get_out = false;
		}
		else {
			std::cout << "Read1 threshold passed\n";
		}
		readsFileName << temp3;

		//COOLDOWN
		if (failTestNum >= 0.000008) {	// >8uA then let it cool down
			std::this_thread::sleep_for(std::chrono::seconds(30));
			std::cout << "30 Second pause\n";
		}
		
		status = viWrite(instr, (ViBuf)"L3E-4,7X", (ViUInt32)strlen("L3E-4,7X"), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q3,5,2,1,400,1000X", (ViUInt32)strlen("Q3,5,2,1,400,1000X"), &writeCount);	//TODO modify reset voltage to possibly +4
		status = viWrite(instr, (ViBuf)"Q9,0.3,2,1,500,500X", (ViUInt32)strlen("Q9,0.3,2,1,500,500X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		//std::this_thread::sleep_for(std::chrono::milliseconds(200));

		//read3
		status = viWrite(instr, (ViBuf)"U11X", (ViUInt32)strlen("U11X"), &writeCount);
		status = viRead(instr, buffer, 100, &retCount);	//reading how much data is available
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.assign(Sbuffer, 3, 4);
		dataToRead = std::stoi(Sbuffer);
		bytesToRead = dataToRead * 13;
		//std::cout << "Data to read: " << dataToRead << std::endl;
		//std::cout << "Bytes to read: " << bytesToRead << std::endl;

		status = viRead(instr, buffer, bytesToRead, &retCount);
		//std::cout << "retCount: " << retCount << " bytes" << std::endl;
		//std::cout << "Data read" << std::endl;
		Sbuffer = reinterpret_cast<char const*>(buffer);
		//Format data into column
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 52);
		outputFileName << Sbuffer;
	//	std::cout << Sbuffer << std::endl;

		temp3.assign(Sbuffer, 26, 12);


		temp1.assign(Sbuffer, 27, 7);
		temp2.assign(Sbuffer, 36, 2);
	//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
		failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
	//	std::cout << "Read2 current: " << failTestNum << " Amps\n";	//for testing. check Sbuffers arrigns of temp1 and temp2 to debug;
		if (failTestNum >= 0.00000019f) {	//More than 0.5uA
			while (!get_out) {
				std::cout << "Read2 threshold failed\n";
				status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,5,2,1,1000,200X", (ViUInt32)strlen("Q3,5,2,1,1000,200X"), &writeCount);
				status = viWrite(instr, (ViBuf)"Q9,0.3,2,1,500,500X", (ViUInt32)strlen("Q9,0.3,2,1,500,500X"), &writeCount);
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::cout << "- - - - - - - Applied 4mA RESET- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(2));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));


				//read3
				status = viWrite(instr, (ViBuf)"U11X", (ViUInt32)strlen("U11X"), &writeCount);
				status = viRead(instr, buffer, 100, &retCount);	//reading how much data is available
				Sbuffer = reinterpret_cast<char const*>(buffer);
				Sbuffer.assign(Sbuffer, 3, 4);
				dataToRead = std::stoi(Sbuffer);
				bytesToRead = dataToRead * 13;
				//std::cout << "Data to read: " << dataToRead << std::endl;
				//std::cout << "Bytes to read: " << bytesToRead << std::endl;

				status = viRead(instr, buffer, bytesToRead, &retCount);
				//std::cout << "retCount: " << retCount << " bytes" << std::endl;
				//std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 52);
				outputFileName << Sbuffer;
				assist_forming_counter_file << i + 1 << "R ";
			//	std::cout << Sbuffer << std::endl;

				temp3.assign(Sbuffer, 26, 12);
				temp1.assign(Sbuffer, 27, 7);
				temp2.assign(Sbuffer, 36, 2);
			//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
				failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
			//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

				if (failTestNum <= 0.00000019f) {
					get_out = true;
					std::cout << "Read2 threshold passed with help1\n";
				}
				else {	//help2
					std::cout << "Read2 threshold failed\n";
					status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
					status = viWrite(instr, (ViBuf)"Q3,5,2,1,1000,200X", (ViUInt32)strlen("Q3,5,2,1,1000,200X"), &writeCount);
					status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount); 
					std::cout << "- - - - - - - Applied 50mA RESET- - - - - -\n\n";
					std::this_thread::sleep_for(std::chrono::seconds(1));
					std::this_thread::sleep_for(std::chrono::milliseconds(600));


					//NEED 2 parts, so I don''t get "00.0...1"
					//read part 1

					status = viRead(instr, buffer, bytesToRead, &retCount);
					//std::cout << "retCount: " << retCount << " bytes" << std::endl;
					//std::cout << "Data read" << std::endl;
					Sbuffer = reinterpret_cast<char const*>(buffer);
					//Format data into column
					position = Sbuffer.find(",");
					while (position != std::string::npos) {
						Sbuffer.replace(position, 1, "\n");
						position = Sbuffer.find(",", position + 1);
					}
					Sbuffer.assign(Sbuffer, 0, 26);
					outputFileName << Sbuffer;

					//read part 2
					status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
					status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
					status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
					std::this_thread::sleep_for(std::chrono::seconds(1));
					std::this_thread::sleep_for(std::chrono::milliseconds(300));

					status = viRead(instr, buffer, bytesToRead, &retCount);
					Sbuffer = reinterpret_cast<char const*>(buffer);
					position = Sbuffer.find(",");
					while (position != std::string::npos) {
						Sbuffer.replace(position, 1, "\n");
						position = Sbuffer.find(",", position + 1);
					}
					Sbuffer.assign(Sbuffer, 0, 26);
					outputFileName << Sbuffer;

					assist_forming_counter_file << i + 1 << "R ";
				//	std::cout << Sbuffer << std::endl;

					temp3.assign(Sbuffer, 0, 12);
					temp1.assign(Sbuffer, 1, 7);
					temp2.assign(Sbuffer, 10, 2);
				//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
					failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
				//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

					if (failTestNum <= 0.00000019f) {
						get_out = true;
						std::cout << "Read2 threshold passed with help2\n";
					}
					else {	//help3
						std::cout << "Read2 threshold failed\n";
						status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
						status = viWrite(instr, (ViBuf)"Q3,7,2,1,1000,200X", (ViUInt32)strlen("Q3,5,2,1,1000,200X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::cout << "- - - - - - - Applied 50mA, 7V RESET- - - - - -\n\n";
						std::this_thread::sleep_for(std::chrono::seconds(1));
						std::this_thread::sleep_for(std::chrono::milliseconds(600));


						//read3

						status = viRead(instr, buffer, bytesToRead, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);
						outputFileName << Sbuffer;

						//read part 2
						status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
						status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(1));
						std::this_thread::sleep_for(std::chrono::milliseconds(300));

						status = viRead(instr, buffer, bytesToRead, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);
						outputFileName << Sbuffer;

						assist_forming_counter_file << i + 1 << "R ";
					//	std::cout << Sbuffer << std::endl;

						temp3.assign(Sbuffer, 0, 12);
						temp1.assign(Sbuffer, 1, 7);
						temp2.assign(Sbuffer, 10, 2);

						//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
						failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
					//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

						if (failTestNum <= 0.00000019f) {
							get_out = true;
							std::cout << "Read2 threshold passed with help3\n";
						}
						else {	//help4
							std::cout << "Read2 threshold failed\n";
							status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
							status = viWrite(instr, (ViBuf)"Q3,12,2,1,1000,200X", (ViUInt32)strlen("Q3,12,2,1,1000,200X"), &writeCount);
							status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
							std::cout << "- - - - - - - Applied 50mA, 7V RESET- - - - - -\n\n";
							std::this_thread::sleep_for(std::chrono::seconds(2));


							//read3

							status = viRead(instr, buffer, bytesToRead, &retCount);
							Sbuffer = reinterpret_cast<char const*>(buffer);
							position = Sbuffer.find(",");
							while (position != std::string::npos) {
								Sbuffer.replace(position, 1, "\n");
								position = Sbuffer.find(",", position + 1);
							}
							Sbuffer.assign(Sbuffer, 0, 26);
							outputFileName << Sbuffer;

							//read part 2
							status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
							status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
							status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
							std::this_thread::sleep_for(std::chrono::seconds(1));
							std::this_thread::sleep_for(std::chrono::milliseconds(300));

							status = viRead(instr, buffer, bytesToRead, &retCount);
							Sbuffer = reinterpret_cast<char const*>(buffer);
							position = Sbuffer.find(",");
							while (position != std::string::npos) {
								Sbuffer.replace(position, 1, "\n");
								position = Sbuffer.find(",", position + 1);
							}
							Sbuffer.assign(Sbuffer, 0, 26);
							outputFileName << Sbuffer;

							assist_forming_counter_file << i + 1 << "R ";
							//	std::cout << Sbuffer << std::endl;

							temp3.assign(Sbuffer, 0, 12);
							temp1.assign(Sbuffer, 1, 7);
							temp2.assign(Sbuffer, 10, 2);

							//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
							failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
							//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

							if (failTestNum <= 0.00000019f) {
								get_out = true;
								std::cout << "Read2 threshold passed with help3\n";
							}
						}
					}
				}
			}
			get_out = false;
		}
		else {
			std::cout << "Read2 threshold passed\n";
		}
		readsFileName << "\t" << temp3 << std::endl;
		assist_forming_counter_file << std::endl;
		outputFileName.flush();
		std::cout << " - - - - - - Run #" << i+1 << " out of " << amountOfRuns << " complete - - - - - - \n\n";
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void small_device_pulsed_mode_proper_forming() {
	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	bool enable_healing;
	while (1) {
		std::cout << "Enable healing? 1/0\n";
		std::cin >> enable_healing;
		if (enable_healing) {
			std::cout << "Healing enabled\n";
			break;
		}
		else if (!enable_healing) {
			std::cout << "Healing disabled\n";
			break;
		}
		std::cout << "Wrong input\n";
	}
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	std::ofstream readsFileName("reads.txt", std::ios::app);
	std::ofstream assist_forming_counter_file("afc.txt", std::ios::app);
	float failTestNum;
	std::string temp1;
	std::string temp2;
	std::string temp3;
	std::string::size_type sz;
	bool get_out = false;
	int set_fail_counter = 0;
	int position;
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0,0,0X", (ViUInt32)strlen("B0,0,0X"), &writeCount);
//	status = viWrite(instr, (ViBuf)"L0.1E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);	//CC Settings; not necessary here, but the instrument doesn't argue.
	status = viWrite(instr, (ViBuf)"F0,1X", (ViUInt32)strlen("F0,1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"G4,2,2X", (ViUInt32)strlen("G4,2,2X"), &writeCount);
//	status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);	//create sweep
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	for (int i = 0; i < amountOfRuns; ++i) {
		status = viWrite(instr, (ViBuf)"L0.2E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q3,5,2,1,500,100X", (ViUInt32)strlen("Q3,5,2,1,100,100X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q9,-5,2,1,100,100X", (ViUInt32)strlen("Q3,-5,2,1,100,100X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

		//read part 1
		status = viRead(instr, buffer, 52, &retCount);
		//std::cout << "retCount: " << retCount << " bytes" << std::endl;
		//std::cout << "Data read" << std::endl;
		Sbuffer = reinterpret_cast<char const*>(buffer);
		//Format data into column
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 52);
		outputFileName << Sbuffer;
		//read part 2
		status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		status = viRead(instr, buffer, 26, &retCount);
		Sbuffer = reinterpret_cast<char const*>(buffer);
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 26);
		outputFileName << Sbuffer;

	//	std::cout << Sbuffer << std::endl;

		temp3.assign(Sbuffer, 0, 12);
		temp1.assign(Sbuffer, 1, 7);
		temp2.assign(Sbuffer, 10, 2);
	//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
		failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
		std::cout << "Read1 current: " << failTestNum << " Amps\n";
		
		/*							//TODO implement failproof
		if (failTestNum <= 0.00000039f) {	//Less than 0.4uA
			while (!get_out) {
				set_fail_counter++;
				std::cout << "Read1 threshold failed\n";
				status = viWrite(instr, (ViBuf)"L0.2E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,4,2,1,3000,200X", (ViUInt32)strlen("Q3,4,2,1,3000,200X"), &writeCount);	//create sweep
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::cout << "- - - - - - - Applied +V- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(4));
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				//read part 1
				status = viRead(instr, buffer, 26, &retCount);
				//std::cout << "retCount: " << retCount << " bytes" << std::endl;
				//std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;
				status = viWrite(instr, (ViBuf)"L0.2E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,-4,2,1,500,200X", (ViUInt32)strlen("Q3,-4,2,1,500,200X"), &writeCount);	//create sweep
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::cout << "- - - - - - - Applied -V- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				//read part 1
				status = viRead(instr, buffer, 26, &retCount);
				//std::cout << "retCount: " << retCount << " bytes" << std::endl;
				//std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;


				//read part 2
				status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				status = viRead(instr, buffer, 26, &retCount);
				Sbuffer = reinterpret_cast<char const*>(buffer);
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;

				assist_forming_counter_file << i + 1 << "S ";
				//	std::cout << Sbuffer << std::endl;

				temp3.assign(Sbuffer, 0, 12);
				temp1.assign(Sbuffer, 1, 7);
				temp2.assign(Sbuffer, 10, 2);
				//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
				failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
				std::cout << "Read1 current: " << failTestNum << " Amps\n";
				if (failTestNum >= 0.00000099f) {
					get_out = true;
					std::cout << "Read1 threshold passed with help\n";
				}
			}
		}
		
		/*		//CURRENT UP (not sure about it!)
		if (failTestNum <= 0.00000999f ) {	//Less than 10uA
			while (!get_out) {
				set_fail_counter++;
				std::cout << "Read1 threshold to be improved\n";
				status = viWrite(instr, (ViBuf)"L0.1E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,3,2,1,1500,200X", (ViUInt32)strlen("Q3,5,2,1,2500,200X"), &writeCount);	//create sweep
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::cout << "- - - - - - - Applied +V- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(4));
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				//read part 1
				status = viRead(instr, buffer, 26, &retCount);
				//std::cout << "retCount: " << retCount << " bytes" << std::endl;
				//std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;
				//read part 2
				status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				status = viRead(instr, buffer, 26, &retCount);
				Sbuffer = reinterpret_cast<char const*>(buffer);
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;

				assist_forming_counter_file << i + 1 << " ";
				//	std::cout << Sbuffer << std::endl;

				temp3.assign(Sbuffer, 0, 12);
				temp1.assign(Sbuffer, 1, 7);
				temp2.assign(Sbuffer, 10, 2);
				//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
				failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
				std::cout << "Read1 current: " << failTestNum << " Amps\n";
			//	if (failTestNum >= 0.00000499f) {
					get_out = true;
					std::cout << "Read1 threshold passed with help\n";
			//	}
				if (set_fail_counter >= 30 && enable_healing) {	//HEALING EXPERIMENTAL
					std::cout << "Set Failed 30 times\nPerforming healing\n";
					while (1) {
						status = viWrite(instr, (ViBuf)"L8E-4,7X", (ViUInt32)strlen("L8E-4,7X"), &writeCount);
						status = viWrite(instr, (ViBuf)"Q3,5,2,1,1000,1000X", (ViUInt32)strlen("Q3,5,2,1,1000,1000X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(2));
						std::this_thread::sleep_for(std::chrono::milliseconds(300));
						status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);
						status = viWrite(instr, (ViBuf)"Q3,5,2,1,300,500X", (ViUInt32)strlen("Q3,5,2,1,300,500X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(1));
						status = viWrite(instr, (ViBuf)"L0.1E-3,7X", (ViUInt32)strlen("L0.1E-3,7X"), &writeCount);
						status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(1));
						std::this_thread::sleep_for(std::chrono::milliseconds(200));
						status = viRead(instr, buffer, 26, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);

						assist_forming_counter_file << i + 1 << "H ";
						temp3.assign(Sbuffer, 0, 12);
						temp1.assign(Sbuffer, 1, 7);
						temp2.assign(Sbuffer, 10, 2);
						//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
						failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
						//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

						if (failTestNum <= 0.00000019f) {
							while (!get_out) {
								status = viWrite(instr, (ViBuf)"Q3,5,2,1,1500,200X", (ViUInt32)strlen("Q3,5,2,1,2500,200X"), &writeCount);	//create sweep
								status = viWrite(instr, (ViBuf)"Q9,-5,2,1,500,500X", (ViUInt32)strlen("Q9,-5,2,1,500,500X"), &writeCount);
								status = viWrite(instr, (ViBuf)"Q9,0.3,2,1,500,500X", (ViUInt32)strlen("Q9,0.3,2,1,500,500X"), &writeCount);
								status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
								std::this_thread::sleep_for(std::chrono::seconds(4));
								std::this_thread::sleep_for(std::chrono::milliseconds(500));
								status = viRead(instr, buffer, bytesToRead, &retCount);
								//	std::cout << "retCount: " << retCount << " bytes" << std::endl;
								//	std::cout << "Data read" << std::endl;
								Sbuffer = reinterpret_cast<char const*>(buffer);
								//Format data into column
								position = Sbuffer.find(",");
								while (position != std::string::npos) {
									Sbuffer.replace(position, 1, "\n");
									position = Sbuffer.find(",", position + 1);
								}
								Sbuffer.assign(Sbuffer, 0, 78);
								outputFileName << Sbuffer;
								assist_forming_counter_file << i + 1 << "H ";
								//	std::cout << Sbuffer << std::endl;

								temp1.assign(Sbuffer, 53, 7);
								temp2.assign(Sbuffer, 62, 2);
								failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
								//	std::cout << "Read1 current after +5V help: " << failTestNum << " Amps\n";	//for testing. check Sbuffers arrigns of temp1 and temp2 to debug;
								if (failTestNum >= 0.00000099f) {
									get_out = true;
									temp3.assign(Sbuffer, 52, 12);
								}
							}
							std::cout << "Healing procedure performed\n";
							break;
						}
					}
				}
			}
			// end comment here
		else {
			std::cout << "Read1 threshold passed\n";
		}
		*/
		readsFileName << temp3;
		get_out = false;
		
		//COOLDOWN
		if (failTestNum >= 0.000005) {	// >5uA then let it cool down
			std::this_thread::sleep_for(std::chrono::seconds(2));
			std::cout << "2 Second pause\n";
		}

		status = viWrite(instr, (ViBuf)"L1E-1,9X", (ViUInt32)strlen("L1E-1,9X"), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q3,5,2,1,100,1000X", (ViUInt32)strlen("Q3,5,2,1,400,1000X"), &writeCount);	//TODO modify reset voltage to possibly +4
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		//std::this_thread::sleep_for(std::chrono::milliseconds(200));

		//read part 1

		status = viRead(instr, buffer, 26, &retCount);
		//std::cout << "retCount: " << retCount << " bytes" << std::endl;
		//std::cout << "Data read" << std::endl;
		Sbuffer = reinterpret_cast<char const*>(buffer);
		//Format data into column
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 26);
		outputFileName << Sbuffer;

		std::this_thread::sleep_for(std::chrono::seconds(1));

		//read part 2
		status = viWrite(instr, (ViBuf)"L1E-6,4X", (ViUInt32)strlen("L1E-6,4X"), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,1000,500X", (ViUInt32)strlen("Q3,0.3,2,1,1000,500X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(3));
	//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

		status = viRead(instr, buffer, 26, &retCount);
		Sbuffer = reinterpret_cast<char const*>(buffer);
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 26);
		outputFileName << Sbuffer;

		//	std::cout << Sbuffer << std::endl;

		temp3.assign(Sbuffer, 0, 12);
		temp1.assign(Sbuffer, 1, 7);
		temp2.assign(Sbuffer, 10, 2);
		//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
		failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
		std::cout << "Read2 current: " << failTestNum << " Amps\n";
		/*
		if (failTestNum >= 0.000000199f) {	//More than 0.2uA
			while (!get_out) {
				std::cout << "Read2 threshold failed\n";
				status = viWrite(instr, (ViBuf)"L1E-3,7X", (ViUInt32)strlen("L1E-3,7X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,5,2,1,200,1000X", (ViUInt32)strlen("Q3,5,2,1,200,1000X"), &writeCount);
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::cout << "- - - - - - - Applied 1mA CC RESET- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(3));
			//	std::this_thread::sleep_for(std::chrono::milliseconds(500));
				//read part 1

				status = viRead(instr, buffer, 26, &retCount);
				//std::cout << "retCount: " << retCount << " bytes" << std::endl;
				//std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;

				//read part 2
				status = viWrite(instr, (ViBuf)"L1E-6,4X", (ViUInt32)strlen("L1E-6,4X"), &writeCount);	//CC Settings
				status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,1000,500X", (ViUInt32)strlen("Q3,0.3,2,1,1000,500X"), &writeCount);
				status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
				std::this_thread::sleep_for(std::chrono::seconds(3));
			//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

				status = viRead(instr, buffer, 26, &retCount);
				Sbuffer = reinterpret_cast<char const*>(buffer);
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;

				assist_forming_counter_file << i + 1 << "R ";
				//	std::cout << Sbuffer << std::endl;

				temp3.assign(Sbuffer, 0, 12);
				temp1.assign(Sbuffer, 1, 7);
				temp2.assign(Sbuffer, 10, 2);
				//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
				failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
				std::cout << "Read2 current after 1mA RESET: " << failTestNum << " Amps\n";

				if (failTestNum <= 0.00000019f) {
					get_out = true;
					std::cout << "Read2 threshold passed with help1\n";
				}
				
				else {	//help2
					std::cout << "Read2 threshold failed\n";
					status = viWrite(instr, (ViBuf)"L1E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
					status = viWrite(instr, (ViBuf)"Q3,5,2,1,1000,200X", (ViUInt32)strlen("Q3,5,2,1,1000,200X"), &writeCount);
					status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
					std::cout << "- - - - - - - Applied 10mA RESET- - - - - -\n\n";
					std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(600));


					//NEED 2 parts, so I don''t get "00.0...1"
					//read part 1

					status = viRead(instr, buffer, 26, &retCount);
					//std::cout << "retCount: " << retCount << " bytes" << std::endl;
					//std::cout << "Data read" << std::endl;
					Sbuffer = reinterpret_cast<char const*>(buffer);
					//Format data into column
					position = Sbuffer.find(",");
					while (position != std::string::npos) {
						Sbuffer.replace(position, 1, "\n");
						position = Sbuffer.find(",", position + 1);
					}
					Sbuffer.assign(Sbuffer, 0, 26);
					outputFileName << Sbuffer;

					//read part 2
					status = viWrite(instr, (ViBuf)"L1E-6,4X", (ViUInt32)strlen("L1E-6,4X"), &writeCount);	//CC Settings
					status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
					status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
					std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

					status = viRead(instr, buffer, 26, &retCount);
					Sbuffer = reinterpret_cast<char const*>(buffer);
					position = Sbuffer.find(",");
					while (position != std::string::npos) {
						Sbuffer.replace(position, 1, "\n");
						position = Sbuffer.find(",", position + 1);
					}
					Sbuffer.assign(Sbuffer, 0, 26);
					outputFileName << Sbuffer;

					assist_forming_counter_file << i + 1 << "R ";
					//	std::cout << Sbuffer << std::endl;

					temp3.assign(Sbuffer, 0, 12);
					temp1.assign(Sbuffer, 1, 7);
					temp2.assign(Sbuffer, 10, 2);
					//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
					failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
					std::cout << "Read2 current after 10mA RESET: " << failTestNum << " Amps\n";

					if (failTestNum <= 0.00000019f) {
						get_out = true;
						std::cout << "Read2 threshold passed with help2\n";
					}

					
					else {	//help3
						std::cout << "Read2 threshold failed\n";
						status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
						status = viWrite(instr, (ViBuf)"Q3,5,2,1,200,1000X", (ViUInt32)strlen("Q3,5,2,1,200,1000X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::cout << "- - - - - - - Applied 50mA, 4V RESET- - - - - -\n\n";
						std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(600));


						//read3

						status = viRead(instr, buffer, 26, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);
						outputFileName << Sbuffer;

						//read part 2
						status = viWrite(instr, (ViBuf)"L1E-6,4X", (ViUInt32)strlen("L1E-6,4X"), &writeCount);	//CC Settings
						status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

						status = viRead(instr, buffer, 26, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);
						outputFileName << Sbuffer;

						assist_forming_counter_file << i + 1 << "R ";
						//	std::cout << Sbuffer << std::endl;

						temp3.assign(Sbuffer, 0, 12);
						temp1.assign(Sbuffer, 1, 7);
						temp2.assign(Sbuffer, 10, 2);

						//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
						failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
						std::cout << "Read2 current after 50mA RESET: " << failTestNum << " Amps\n";

						if (failTestNum <= 0.00000019f) {
							get_out = true;
							std::cout << "Read2 threshold passed with help3\n";
						}
						/*
						else {	//help4
							std::cout << "Read2 threshold failed\n";
							status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
							status = viWrite(instr, (ViBuf)"Q3,7,2,1,1000,200X", (ViUInt32)strlen("Q3,7,2,1,1000,200X"), &writeCount);
							status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
							std::cout << "- - - - - - - Applied 50mA, 7V RESET- - - - - -\n\n";
							std::this_thread::sleep_for(std::chrono::seconds(2));


							//read3

							status = viRead(instr, buffer, 26, &retCount);
							Sbuffer = reinterpret_cast<char const*>(buffer);
							position = Sbuffer.find(",");
							while (position != std::string::npos) {
								Sbuffer.replace(position, 1, "\n");
								position = Sbuffer.find(",", position + 1);
							}
							Sbuffer.assign(Sbuffer, 0, 26);
							outputFileName << Sbuffer;

							//read part 2
							status = viWrite(instr, (ViBuf)"L1E-6,4X", (ViUInt32)strlen("L1E-6,4X"), &writeCount);	//CC Settings
							status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
							status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
							std::this_thread::sleep_for(std::chrono::seconds(1));
							std::this_thread::sleep_for(std::chrono::milliseconds(300));

							status = viRead(instr, buffer, 26, &retCount);
							Sbuffer = reinterpret_cast<char const*>(buffer);
							position = Sbuffer.find(",");
							while (position != std::string::npos) {
								Sbuffer.replace(position, 1, "\n");
								position = Sbuffer.find(",", position + 1);
							}
							Sbuffer.assign(Sbuffer, 0, 26);
							outputFileName << Sbuffer;

							assist_forming_counter_file << i + 1 << "R ";
							//	std::cout << Sbuffer << std::endl;

							temp3.assign(Sbuffer, 0, 12);
							temp1.assign(Sbuffer, 1, 7);
							temp2.assign(Sbuffer, 10, 2);

							//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
							failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
							//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

							if (failTestNum <= 0.00000019f) {
								get_out = true;
								std::cout << "Read2 threshold passed with help3\n";
							}
						}
					//	do end comment here
					}
					
				}
				
			}
			get_out = false;
		}
		else {
			std::cout << "Read2 threshold passed\n";
		}

		*/
		readsFileName << "\t" << temp3 << std::endl;
		assist_forming_counter_file << std::endl;
		outputFileName.flush();
		std::cout << " - - - - - - Run #" << i + 1 << " out of " << amountOfRuns << " complete - - - - - - \n\n";
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void pulses_failure_assistSmart() {
	std::this_thread::sleep_for(std::chrono::seconds(1));
	OPERATE_OFF
	MODE_DC
	TRIGGER_ACTION
//	DATA_FORMAT_OUTPUT_TICK
	set_dc_bias(4.0f,0);
	setCurrentCompliance("4E-4",0);
	OPERATE_ON
	std::cout << "40 seconds forming ongoing" << std::endl;
	waitAndPrintProgress(40);
	set_dc_bias(0.3f, 0);
	std::cout << "10 seconds resting" << std::endl;
	waitAndPrintProgress(10);
	OPERATE_OFF
	std::cout << "\nforming finished" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	set_dc_bias(0.0f, 0);
}

void test0_pulsed_modeSmart() {
	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	std::ofstream readsFileName("reads.txt", std::ios::app);
	std::ofstream assist_forming_counter_file("afc.txt", std::ios::app);
	float failTestNum;
	static std::string temp1;
	static std::string temp2;
	static std::string temp3;
	std::string::size_type sz;
	bool get_out = false;
	int position;
	setCurrentCompliance("4E-4", 7);	//IMPORTANT if not here,the instrument starts saying PULSE TIME NOT MET
	set_dc_bias(0.0f, 0);
	INTEGRATION_TIME_FAST
	FILTER_DISABLE
	OPERATE_OFF
	TRIGGER_DISABLE
	MODE_SWEEP
	DATA_FORMAT_OUTPUT_SWEEP
	OPERATE_ON
	TRIGGER_ENABLE
	for (int i = 0; i < amountOfRuns; ++i) {
		setCurrentCompliance("4E-4", 7);
	//	status = viWrite(instr, (ViBuf)"Q3,5,2,1,500,100X", (ViUInt32)strlen("Q3,5,2,1,100,100X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q3,-4,2,1,100,100X", (ViUInt32)strlen("Q3,-4,2,1,100,100X"), &writeCount);
		TRIGGER_ACTION
		std::this_thread::sleep_for(std::chrono::seconds(1));
		//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

			//read part 1
		status = viRead(instr, buffer, 26, &retCount);
		//std::cout << "retCount: " << retCount << " bytes" << std::endl;
		//std::cout << "Data read" << std::endl;
		Sbuffer = reinterpret_cast<char const*>(buffer);
		//Format data into column
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 26);
		outputFileName << Sbuffer;
		//read part 2
		setCurrentCompliance("1E-3", 7);
		status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
		TRIGGER_ACTION
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		status = viRead(instr, buffer, 26, &retCount);
		Sbuffer = reinterpret_cast<char const*>(buffer);
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 26);
		outputFileName << Sbuffer;

		//	std::cout << Sbuffer << std::endl;

		temp3.assign(Sbuffer, 0, 12);
		temp1.assign(Sbuffer, 1, 7);
		temp2.assign(Sbuffer, 10, 2);
		//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
		failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
		std::cout << "Read1 current: " << failTestNum * pow( 10, 6 ) << " uAmps\n";

		if (failTestNum <= 0.00000079f) {	//Less than 0.4uA
			while (!get_out) {
				std::cout << "Read1 threshold failed\n";
				pulses_failure_assistSmart();
				MODE_SWEEP
				OPERATE_ON
				assist_forming_counter_file << i + 1 << " ";
				setCurrentCompliance("4E-4", 7);
	//		status = viWrite(instr, (ViBuf)"Q3,5,2,1,500,100X", (ViUInt32)strlen("Q3,5,2,1,100,100X"), &writeCount);
				status = viWrite(instr, (ViBuf)"Q3,-4,2,1,100,100X", (ViUInt32)strlen("Q3,-4,2,1,100,100X"), &writeCount);
				TRIGGER_ACTION
				std::this_thread::sleep_for(std::chrono::seconds(1));
			//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

				//read part 1
				status = viRead(instr, buffer, 26, &retCount);
				//std::cout << "retCount: " << retCount << " bytes" << std::endl;
				//std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;
				//read part 2
				setCurrentCompliance("1E-3", 7);
				status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
				TRIGGER_ACTION
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				status = viRead(instr, buffer, 26, &retCount);
				Sbuffer = reinterpret_cast<char const*>(buffer);
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;

				//	std::cout << Sbuffer << std::endl;

				temp3.assign(Sbuffer, 0, 12);
				temp1.assign(Sbuffer, 1, 7);
				temp2.assign(Sbuffer, 10, 2);
				//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
				failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
				std::cout << "Read1 current: " << failTestNum * pow(10, 6) << " uAmps\n";

				if (failTestNum >= 0.00000099f) {
					get_out = true;
					std::cout << "Read1 threshold passed with help\n";
				}
			}
		}

		readsFileName << temp3;
		get_out = false;

		//COOLDOWN
		if (failTestNum >= 0.000005) {	// >5uA then let it cool down
			std::this_thread::sleep_for(std::chrono::seconds(2));
			std::cout << "2 Second pause\n";
		}

		setCurrentCompliance("1E-1", 9);
		status = viWrite(instr, (ViBuf)"Q3,3.5,2,1,100,1000X", (ViUInt32)strlen("Q3,3.5,2,1,400,1000X"), &writeCount);	//TODO modify reset voltage to possibly +4
		TRIGGER_ACTION
		std::this_thread::sleep_for(std::chrono::seconds(2));
		//std::this_thread::sleep_for(std::chrono::milliseconds(200));

		//read part 1

		status = viRead(instr, buffer, 26, &retCount);
		//std::cout << "retCount: " << retCount << " bytes" << std::endl;
		//std::cout << "Data read" << std::endl;
		Sbuffer = reinterpret_cast<char const*>(buffer);
		//Format data into column
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 26);
		outputFileName << Sbuffer;

		std::this_thread::sleep_for(std::chrono::seconds(1));

		//read part 2
		setCurrentCompliance("1E-6", 4);
		status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
		TRIGGER_ACTION
		std::this_thread::sleep_for(std::chrono::seconds(3));
		//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

		status = viRead(instr, buffer, 26, &retCount);
		Sbuffer = reinterpret_cast<char const*>(buffer);
		position = Sbuffer.find(",");
		while (position != std::string::npos) {
			Sbuffer.replace(position, 1, "\n");
			position = Sbuffer.find(",", position + 1);
		}
		Sbuffer.assign(Sbuffer, 0, 26);
		outputFileName << Sbuffer;

		//	std::cout << Sbuffer << std::endl;

		temp3.assign(Sbuffer, 0, 12);
		temp1.assign(Sbuffer, 1, 7);
		temp2.assign(Sbuffer, 10, 2);
		//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
		failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
		std::cout << "Read2 current: " << failTestNum * pow(10, 9) << " nAmps\n";

		if (failTestNum >= 0.000000199f) {	//More than 0.2uA
			while (!get_out) {
				std::cout << "Read2 threshold failed\n";
				setCurrentCompliance("1E-1", 9);
				status = viWrite(instr, (ViBuf)"Q3,4,2,1,200,1000X", (ViUInt32)strlen("Q3,5,2,1,200,1000X"), &writeCount);
				TRIGGER_ACTION
				std::cout << "- - - - - - - Applied +4V RESET- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(500));
					//read part 1

				status = viRead(instr, buffer, 26, &retCount);
				//std::cout << "retCount: " << retCount << " bytes" << std::endl;
				//std::cout << "Data read" << std::endl;
				Sbuffer = reinterpret_cast<char const*>(buffer);
				//Format data into column
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;

				//read part 2
				setCurrentCompliance("1E-6", 4);
				status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
				TRIGGER_ACTION
				std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

				status = viRead(instr, buffer, 26, &retCount);
				Sbuffer = reinterpret_cast<char const*>(buffer);
				position = Sbuffer.find(",");
				while (position != std::string::npos) {
					Sbuffer.replace(position, 1, "\n");
					position = Sbuffer.find(",", position + 1);
				}
				Sbuffer.assign(Sbuffer, 0, 26);
				outputFileName << Sbuffer;

				assist_forming_counter_file << i + 1 << "R ";
				//	std::cout << Sbuffer << std::endl;

				temp3.assign(Sbuffer, 0, 12);
				temp1.assign(Sbuffer, 1, 7);
				temp2.assign(Sbuffer, 10, 2);
				//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
				failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
				std::cout << "Read2 current after +4V RESET: " << failTestNum * pow(10, 9) << " nAmps\n";

				if (failTestNum <= 0.00000019f) {
					get_out = true;
					std::cout << "Read2 threshold passed with help1\n";
				}
				else {	//help2
					std::cout << "Read2 threshold failed\n";
					setCurrentCompliance("1E-1", 9);
					status = viWrite(instr, (ViBuf)"Q3,5,2,1,1000,200X", (ViUInt32)strlen("Q3,5,2,1,1000,200X"), &writeCount);
					TRIGGER_ACTION
					std::cout << "- - - - - - - Applied +5V RESET- - - - - -\n\n";
					std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(600));


						//NEED 2 parts, so I don''t get "00.0...1"
						//read part 1

					status = viRead(instr, buffer, 26, &retCount);
					//std::cout << "retCount: " << retCount << " bytes" << std::endl;
					//std::cout << "Data read" << std::endl;
					Sbuffer = reinterpret_cast<char const*>(buffer);
					//Format data into column
					position = Sbuffer.find(",");
					while (position != std::string::npos) {
						Sbuffer.replace(position, 1, "\n");
						position = Sbuffer.find(",", position + 1);
					}
					Sbuffer.assign(Sbuffer, 0, 26);
					outputFileName << Sbuffer;

					//read part 2
					setCurrentCompliance("1E-6", 4);
					status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
					TRIGGER_ACTION
					std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

					status = viRead(instr, buffer, 26, &retCount);
					Sbuffer = reinterpret_cast<char const*>(buffer);
					position = Sbuffer.find(",");
					while (position != std::string::npos) {
						Sbuffer.replace(position, 1, "\n");
						position = Sbuffer.find(",", position + 1);
					}
					Sbuffer.assign(Sbuffer, 0, 26);
					outputFileName << Sbuffer;

					assist_forming_counter_file << i + 1 << "R ";
					//	std::cout << Sbuffer << std::endl;

					temp3.assign(Sbuffer, 0, 12);
					temp1.assign(Sbuffer, 1, 7);
					temp2.assign(Sbuffer, 10, 2);
					//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
					failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
					std::cout << "Read2 current after +5V RESET: " << failTestNum * pow(10, 9) << " nAmps\n";

					if (failTestNum <= 0.00000019f) {
						get_out = true;
						std::cout << "Read2 threshold passed with help2\n";
					}
				}
			}
			get_out = false;
		}
/*
					else {	//help3
						std::cout << "Read2 threshold failed\n";
						status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
						status = viWrite(instr, (ViBuf)"Q3,5,2,1,200,1000X", (ViUInt32)strlen("Q3,5,2,1,200,1000X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::cout << "- - - - - - - Applied 50mA, 4V RESET- - - - - -\n\n";
						std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(600));


						//read3

						status = viRead(instr, buffer, 26, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);
						outputFileName << Sbuffer;

						//read part 2
						status = viWrite(instr, (ViBuf)"L1E-6,4X", (ViUInt32)strlen("L1E-6,4X"), &writeCount);	//CC Settings
						status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
						status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
						std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

						status = viRead(instr, buffer, 26, &retCount);
						Sbuffer = reinterpret_cast<char const*>(buffer);
						position = Sbuffer.find(",");
						while (position != std::string::npos) {
							Sbuffer.replace(position, 1, "\n");
							position = Sbuffer.find(",", position + 1);
						}
						Sbuffer.assign(Sbuffer, 0, 26);
						outputFileName << Sbuffer;

						assist_forming_counter_file << i + 1 << "R ";
						//	std::cout << Sbuffer << std::endl;

						temp3.assign(Sbuffer, 0, 12);
						temp1.assign(Sbuffer, 1, 7);
						temp2.assign(Sbuffer, 10, 2);

						//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
						failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
						std::cout << "Read2 current after 50mA RESET: " << failTestNum << " Amps\n";

						if (failTestNum <= 0.00000019f) {
							get_out = true;
							std::cout << "Read2 threshold passed with help3\n";
						}
						/*
						else {	//help4
							std::cout << "Read2 threshold failed\n";
							status = viWrite(instr, (ViBuf)"L5E-2,9X", (ViUInt32)strlen("L5E-2,9X"), &writeCount);	//CC Settings
							status = viWrite(instr, (ViBuf)"Q3,7,2,1,1000,200X", (ViUInt32)strlen("Q3,7,2,1,1000,200X"), &writeCount);
							status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
							std::cout << "- - - - - - - Applied 50mA, 7V RESET- - - - - -\n\n";
							std::this_thread::sleep_for(std::chrono::seconds(2));


							//read3

							status = viRead(instr, buffer, 26, &retCount);
							Sbuffer = reinterpret_cast<char const*>(buffer);
							position = Sbuffer.find(",");
							while (position != std::string::npos) {
								Sbuffer.replace(position, 1, "\n");
								position = Sbuffer.find(",", position + 1);
							}
							Sbuffer.assign(Sbuffer, 0, 26);
							outputFileName << Sbuffer;

							//read part 2
							status = viWrite(instr, (ViBuf)"L1E-6,4X", (ViUInt32)strlen("L1E-6,4X"), &writeCount);	//CC Settings
							status = viWrite(instr, (ViBuf)"Q3,0.3,2,1,500,500X", (ViUInt32)strlen("Q3,0.3,2,1,500,500X"), &writeCount);
							status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
							std::this_thread::sleep_for(std::chrono::seconds(1));
							std::this_thread::sleep_for(std::chrono::milliseconds(300));

							status = viRead(instr, buffer, 26, &retCount);
							Sbuffer = reinterpret_cast<char const*>(buffer);
							position = Sbuffer.find(",");
							while (position != std::string::npos) {
								Sbuffer.replace(position, 1, "\n");
								position = Sbuffer.find(",", position + 1);
							}
							Sbuffer.assign(Sbuffer, 0, 26);
							outputFileName << Sbuffer;

							assist_forming_counter_file << i + 1 << "R ";
							//	std::cout << Sbuffer << std::endl;

							temp3.assign(Sbuffer, 0, 12);
							temp1.assign(Sbuffer, 1, 7);
							temp2.assign(Sbuffer, 10, 2);

							//	std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
							failTestNum = std::stof(temp1, &sz) * pow(10, -1 * std::stoi(temp2, &sz));
							//	std::cout << "Read2 current after 4mA RESET: " << failTestNum << " Amps\n";

							if (failTestNum <= 0.00000019f) {
								get_out = true;
								std::cout << "Read2 threshold passed with help3\n";
							}
						}
					//	do end comment here
					}

				}

			}
			get_out = false;
		}
		else {
			std::cout << "Read2 threshold passed\n";
		}

		*/
		readsFileName << "\t" << temp3 << std::endl;
		assist_forming_counter_file << std::endl;
		outputFileName.flush();
		std::cout << " - - - - - - Run #" << i + 1 << " out of " << amountOfRuns << " complete - - - - - - \n\n";
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void test0neg() {
	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	char str1[30] = "L";
	strcat_s(str1, currentCompliance);
	strcat_s(str1, ",0X");

	std::cout << "Negative up to? (default -3) \n";
	char negative_limit[8];
	std::cin >> negative_limit;
	float Fnegative_limit = strtof(negative_limit, nullptr);
	strcat_s(negative_limit, ",");
	char str2[30] = "Q1,0,";
	strcat_s(str2, negative_limit);

	std::cout << "Voltage step? (default 0.1) \n";
	char voltage_step[20];
	std::cin >> voltage_step;
	strcat_s(str2, voltage_step);
	strcat_s(str2, ",0,");
	float Fvoltage_Step = strtof(voltage_step, nullptr);

	std::cout << "TimeStep? (default 400) \n";
	char timeStep[10];
	std::cin >> timeStep;
	float FtimeStep = strtof(timeStep, nullptr);
	strcat_s(str2, timeStep);
	strcat_s(str2, "X");

	char str3[30] = "Q7,";
	char tt[10];
	sprintf_s(tt, "%0.1f", Fnegative_limit + Fvoltage_Step);
	strcat_s(str3, tt);
	strcat_s(str3, ",0,");
	sprintf_s(tt, "%0.1f", Fvoltage_Step);
	strcat_s(str3, tt);
	strcat_s(str3, ",0,");
	strcat_s(str3, timeStep);
	strcat_s(str3, "X");

	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	status = viWrite(instr, (ViBuf)"L4E-4,0X", (ViUInt32)strlen("L4E-4,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);

	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0,0,0X", (ViUInt32)strlen("B0,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"F0,1X", (ViUInt32)strlen("F0,1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"G4,2,2X", (ViUInt32)strlen("G4,2,2X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	for (int i = 0; i < amountOfRuns; ++i) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		status = viWrite(instr, (ViBuf)str1, (ViUInt32)strlen(str1), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)str2, (ViUInt32)strlen(str2), &writeCount);
		status = viWrite(instr, (ViBuf)str3, (ViUInt32)strlen(str3), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		readSmartFromDevice(std::ceil(Fnegative_limit / Fvoltage_Step) * -2 + 1 + 0, true, 0.005f * FtimeStep, false);	// (-3 * -20 + 1)
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void test0pos() {

	std::cout << "Compliance? XE-Y\n";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	char str1[30] = "L";
	strcat_s(str1, currentCompliance);
	strcat_s(str1, ",0X");

	std::cout << "Voltage step? (default 0.1) \n";
	char voltage_step[20];
	std::cin >> voltage_step;
	float Fvoltage_Step = strtof(voltage_step, nullptr);

	std::cout << "TimeStep? (default 400) \n";
	char timeStep[10];
	std::cin >> timeStep;
	float FtimeStep = strtof(timeStep, nullptr);

	std::cout << "Positive up to? (default 4) \n";
	char positive_limit[5];
	std::cin >> positive_limit;
	float Fpositive_limit = strtof(positive_limit, nullptr);
	strcat_s(positive_limit, ",");
	char str4[30] = "Q1,0,";
	strcat_s(str4, positive_limit);
	strcat_s(str4, voltage_step);
	strcat_s(str4, ",0,");
	strcat_s(str4, timeStep);
	strcat_s(str4, "X");

	char str5[30] = "Q7,";
	char tt[10];
	sprintf_s(tt, "%0.1f", Fpositive_limit - Fvoltage_Step);
	strcat_s(str5, tt);
	strcat_s(str5, ",0,");
	sprintf_s(tt, "%0.1f", Fvoltage_Step);
	strcat_s(str5, tt);
	strcat_s(str5, ",0,");
	strcat_s(str5, timeStep);
	strcat_s(str5, "X");

	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	status = viWrite(instr, (ViBuf)"L4E-4,0X", (ViUInt32)strlen("L4E-4,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0,0,0X", (ViUInt32)strlen("B0,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"F0,1X", (ViUInt32)strlen("F0,1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"G4,2,2X", (ViUInt32)strlen("G4,2,2X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	for (int i = 0; i < amountOfRuns; ++i) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		status = viWrite(instr, (ViBuf)str1, (ViUInt32)strlen(str1), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)str4, (ViUInt32)strlen(str4), &writeCount);
		status = viWrite(instr, (ViBuf)str5, (ViUInt32)strlen(str5), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		readSmartFromDevice(std::ceil(Fpositive_limit / Fvoltage_Step) * 2 + 1, true, 0.003f * FtimeStep, false);
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void smallDeviceForming(){
	float measTime;
	float target;
	bool next_step = false;
	std::string::size_type sz;
	std::ofstream outputFileName("SmallDeviceForming.txt", std::ios::app);
	status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"L0.4E-3,7X", (ViUInt32)strlen("L0.4E-3,7X"), &writeCount);	//CC Settings
	status = viWrite(instr, (ViBuf)"F0,0X", (ViUInt32)strlen("F0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"G15,0,0X", (ViUInt32)strlen("G15,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"B0.3,0,0X", (ViUInt32)strlen("B0.3,0,0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);

	//show initial state
	for (int i = 0; i <= 15; ++i) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
	}
	status = viWrite(instr, (ViBuf)"B3,0,0X", (ViUInt32)strlen("B3,0,0X"), &writeCount);

	while (!next_step) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
		target = std::stof(Sbuffer, &sz);
		if (target >= 0.00038f) {
			next_step = true;
			std::cout << "Step 1 complete!\n";
		}
	}



	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	status = viWrite(instr, (ViBuf)"L2E-3,8X", (ViUInt32)strlen("L2E-3,8X"), &writeCount);	//CC Settings
	status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	status = viWrite(instr, (ViBuf)"B0.3,0,0X", (ViUInt32)strlen("B0.3,0,0X"), &writeCount);
	//show final state
	for (int i = 0; i <= 15; ++i) {
		status = viRead(instr, buffer, 100, &retCount);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		measTime = duration.count();
		Sbuffer = reinterpret_cast<char const*>(buffer);
		Sbuffer.erase(Sbuffer.size() - 22);//remove last letters
		Sbuffer.erase(0, 36);//remove 1st letters
		outputFileName << measTime << "\t" << Sbuffer << std::endl;
	}
	status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
	std::cout << " - - - - - - Small device forming complete - - - - - - \n";
}

int main()
{
	std::cout << "\n\
                              Keithley 237 automation protocol\n\
                                      Pavel Baikov\n\
                                      Version 6.1.2020\n\n";
	connectDevice();

	while (1) {
		std::cin >> userMessage;

		if (userMessage == "status") {
			displayStatus();
		}
		else if(userMessage == "connect") {
			connectDevice();
		}
		else if (userMessage == "disconnect") {
			disconnectDevice();
		}
		else if (userMessage == "write") {
			std::cin >> userMessage;
			writeToDevice(userMessage.c_str());
		}
		else if (userMessage == "readm") {
			readMessageFromDevice();
		}
		else if (userMessage == "test") {
			IV_meas();
		}
		else if (userMessage == "test2") {
			IV_meas2();
		}
		else if (userMessage == "testsmart") {
			IV_measSmart();
		}
		else if (userMessage == "test0neg") {
			test0neg();
		}
		else if (userMessage == "test0pos") {
			test0pos();
		}
		else if (userMessage == "test0pulse") {
			test0_pulsed_modeSmart();
		}
		else if (userMessage == "disco") {
			disco();
		}
		else if (userMessage == "it") {
			Itmeas();
		}
		else if (userMessage == "forming") {
			forming();
		}
		else if (userMessage == "sdforming") {
			smallDeviceForming();
		}
		else if (userMessage == "sdp") {
			small_device_pulsed_mode();
		}
		else if (userMessage == "sdppf") {
			small_device_pulsed_mode_proper_forming();
		}
		else if (userMessage == "ivheat") {
			IV_meas_thermal_Smart();
		}
		else if (userMessage == "exit") {
			disconnectDevice();
			break;
		}
		else {
			std::cout << "Wrong input\n";
		}
	}

	disconnectDevice();
	return 0;
}
