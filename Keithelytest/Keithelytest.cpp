#include <iostream>
#include "visa.h"
#include <bitset>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>

std::string userMessage;

static ViSession defaultRM, instr;
static ViStatus status;
static ViUInt32 writeCount;
static ViUInt32 retCount;
static unsigned char buffer[10000];	// buffer size for data coming from the device
int dataToRead;
int bytesToRead;
std::string Sbuffer(reinterpret_cast<char const*>(buffer));
std::string Devicename = "GPIB0::2::INSTR";


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
	status = viOpen(defaultRM, (char*)Devicename.c_str(), VI_NULL, VI_NULL, &instr);
	std::cout << "Connecting to instrument: " << Devicename << std::endl;
	deviceSettings();	// Apply preset
}

void disconnectDevice() {
	status = viClose(instr);
	std::cout << "Disconnected instrument: " << Devicename << std::endl;
}

void displayStatus() {
	std::cout << "Status: " << status << std::endl;
	std::cout << "Binary Status "<< std::bitset<8>(status) << std::endl;
}

void writeToDevice(char *msg) {
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
	std::cout << "retCount: "  << retCount << " bytes" << std::endl;
	std::cout << "Data in buffer: " << buffer << std::endl;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}

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
	std::cout << "Data in buffer: " << buffer << std::endl;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}
	Sbuffer.erase(Sbuffer.size() - 13);
	outputFileName << Sbuffer;
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
	for (int i = 0; i < amountOfRuns; i++) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		status = viWrite(instr, (ViBuf)"S0X", (ViUInt32)strlen("S0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"P0X", (ViUInt32)strlen("P0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"F0,1X", (ViUInt32)strlen("F0,1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"M2,X", (ViUInt32)strlen("M2,X"), &writeCount);	//Mask to get end of sweep
		status = viWrite(instr, (ViBuf)"G4,2,2X", (ViUInt32)strlen("G4,2,2X"), &writeCount);
		status = viWrite(instr, (ViBuf)str1, (ViUInt32)strlen(str1), &writeCount);	//CC Settings
		status = viWrite(instr, (ViBuf)"Q1,0,-2.2,0.1,0,400X", (ViUInt32)strlen("Q1,0,-2.2,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,-2.1,0,0.1,0,400X", (ViUInt32)strlen("Q7,0,-2.1,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(50));
		status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R0X", (ViUInt32)strlen("R0X"), &writeCount);
		read1FromDevice();
		status = viWrite(instr, (ViBuf)"L1E-1,0X", (ViUInt32)strlen("L1E-1,0X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q1,0.1,2.2,0.1,0,400X", (ViUInt32)strlen("Q1,0.1,2.2,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"Q7,2.1,0,0.1,0,400X", (ViUInt32)strlen("Q7,0,2.1,0.1,0,400X"), &writeCount);
		status = viWrite(instr, (ViBuf)"N1X", (ViUInt32)strlen("N1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"R1X", (ViUInt32)strlen("R1X"), &writeCount);
		status = viWrite(instr, (ViBuf)"H0X", (ViUInt32)strlen("H0X"), &writeCount);
		std::this_thread::sleep_for(std::chrono::seconds(50));
		status = viWrite(instr, (ViBuf)"N0X", (ViUInt32)strlen("N0X"), &writeCount);
		read2FromDevice();
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
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

int main()
{
	status = viOpenDefaultRM(&defaultRM);
	
	while (userMessage != "exit") {
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
			writeToDevice((char*)userMessage.c_str());
		}
		else if (userMessage == "readm") {
			readMessageFromDevice();
		}
		else if (userMessage == "test") {
			IV_meas();
		}
		else if (userMessage == "disco") {
			disco();
		}
		else {
			std::cout << "Wrong input\n";
		}
	}

	disconnectDevice();
	return 0;
}
