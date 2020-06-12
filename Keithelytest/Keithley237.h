#pragma once
#include <iostream>
#include "visa.h"
#include <bitset>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <time.h>
#pragma comment( lib,"visa32" )

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
#define BUFFER_CLEAR				   memset(buffer, 0, sizeof buffer);

class Keithley237 {
public:
	ViStatus writeToInstrument(const char* msg);
	void displayHelp() const;
private:
	std::string userMessage;
	ViSession defaultRM;
	ViSession instr;
	ViStatus status;
	ViUInt32 writeCount;
	ViUInt32 retCount;
	unsigned char buffer[10000];	// buffer size for data coming from the device
	int dataToRead;
	int bytesToRead;
	std::string Sbuffer;
	std::string Devicename = "GPIB0::2::INSTR";
};