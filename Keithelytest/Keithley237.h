#pragma once
#include <iostream>
#include "visa.h"
#include <bitset>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#pragma comment( lib,"visa32" )

#define MODE_DC		                   "F0,0X"
#define MODE_SWEEP	                   "F0,1X"
#define DATA_FORMAT_OUTPUT_TICK        "G15,0,0X"
#define DATA_FORMAT_OUTPUT_SWEEP       "G4,2,2X"
#define DATA_FORMAT_OUTPUT_SWEEP_IV	   "G5,2,2X"
#define TRIGGER_ACTION		           "H0X"
#define OPERATE_OFF                    "N0X"
#define OPERATE_ON	                   "N1X"
#define FILTER_DISABLE		           "P0X"
#define FILTER_2READINGS	           "P1X"
#define FILTER_4READINGS	           "P2X"
#define FILTER_8READINGS	           "P3X"
#define FILTER_16READINGS	           "P4X"
#define FILTER_32READINGS	           "P5X"
#define TRIGGER_ENABLE                 "R1X"
#define TRIGGER_DISABLE                "R0X"
#define INTEGRATION_TIME_FAST          "S0X"	//  416 uSec |4-digit Resolution
#define INTEGRATION_TIME_MEDIUM        "S1X"	//    4 mSec |5-digit Resolution
#define INTEGRATION_TIME_LINECYCLE60HZ "S2X"	//16.67 mSec |5-digit Resolution
#define INTEGRATION_TIME_LINECYCLE50HZ "S3X"	//   20 mSec |5-digit Resolution
#define BUFFER_CLEAR				   std::memset(buffer, '\0', sizeof(buffer));

class Keithley237 {
public:
	Keithley237(const char* name);
	~Keithley237();
	ViStatus connect();
	void disconnect();
	void readInstrumentBuffer();
	std::string& readInstrumentBuffer(std::ofstream& file);
	ViStatus writeToDevice(const char* msg);	//rename to Instrument
	void set_dc_bias(float bias, int range);
	void setCurrentCompliance(const char* value, int range);
	bool readFromInstrument(int data, bool wait, float wait_multiplier, bool do_analysis_of_RonRoff);
	void readSweepFromInstrument(int data, bool wait);
	void readImmediateCurrentValue(std::ofstream& file);
	ViStatus setFlag(const char* flag);
	void displayStatus() const;
private:
	void settings();
private:
	std::string userMessage;
	ViSession defaultRM;
	ViSession instr;	//	intrument handle
	ViStatus status;	//	last error code
	ViUInt32 writeCount;
	ViUInt32 retCount;	//  Amount of bytes read from instrument
	unsigned char buffer[10000];	// program buffer for ASCII data coming from the device
	std::string Sbuffer;	//	alternative buffer to work with strings
	std::string Devicename;
};