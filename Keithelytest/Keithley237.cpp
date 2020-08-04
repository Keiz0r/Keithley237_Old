#include "Keithley237.h"
#include "KeithleyScript.h"

ViStatus Keithley237::writeToDevice(const char* msg) {
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

Keithley237::Keithley237(const char* name)
:
	Devicename(name)
{
	connect();
}

Keithley237::~Keithley237() {
	disconnect();
}

ViStatus Keithley237::connect() {
	status = viOpenDefaultRM(&defaultRM);
	if (status < VI_SUCCESS) {
		std::cerr << "\nERROR Initializing VISA driver" << std::endl;
	}
	status = viOpen(defaultRM, Devicename.c_str(), VI_NULL, VI_NULL, &instr);
	std::cout << "Connecting to instrument: " << Devicename << std::endl;
	if (status < 0) {
		std::cerr << "\n\n\n\n          >>>ERROR: Instrument not found<<<\n\n\n\n\n          Check connection and restart the proram" << std::endl;
		return status;
	}
	else {
		settings();	// Apply preset
		return status;
	}
}

void Keithley237::disconnect() {
	status = viClose(instr);
	status = viClose(defaultRM);
	std::cout << "Disconnected instrument: " << Devicename << std::endl;
}

void Keithley237::readInstrumentBuffer() {
	BUFFER_CLEAR
	status = viRead(instr, buffer, sizeof(buffer), &retCount);
	std::cout << "Message: " << buffer << std::endl;
}

std::string& Keithley237::readInstrumentBuffer(std::ofstream& file) {
	status = viRead(instr, buffer, sizeof(buffer), &retCount);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	bool flipper = false;
	int cntr = 1;
	for (auto iter = Sbuffer.find(','); iter != std::string::npos; iter = Sbuffer.find(',')) {
		if (!flipper) {
			Sbuffer.replace(iter, 1, " ");
		}
		else {
			Sbuffer.replace(iter, 1, "\n");
		}
		flipper = !flipper;
		cntr++;
	}
	file << Sbuffer;
	return Sbuffer;
}

void Keithley237::settings() {
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

void Keithley237::set_dc_bias(float bias, int range) {
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

void Keithley237::setCurrentCompliance(const char* value, int range) {
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

bool Keithley237::readFromInstrument(int data, bool wait, float wait_multiplier, bool do_analysis_of_RonRoff) {
	if (wait) {
		if (data < 0) {
			data = data * -1;
		}
		std::cout << "Expecting to Read: " << data << " data points" << std::endl;
		int time = (int)std::ceil(data * wait_multiplier);
		Script::waitAndPrintProgress(time);
	}
	BUFFER_CLEAR
	status = viRead(instr, buffer, sizeof(buffer), &retCount);
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
		std::cout << "\nON current = " << Ron * pow(10, 6) << " uA" << std::endl;
		std::cout << "OFF current= " << Roff * pow(10, 6) << " uA" << std::endl;
		std::cout << "Ron/Roff ratio= " << Ron / Roff << std::endl;
		if ((Ron / Roff) < 3.0f) {
			std::cout << "!!! Alert, failure encountered !!!  Ron/Roff < 3" << std::endl;
			return false;
		}
	}
	return true;
}

void Keithley237::readSweepFromInstrument(int data, bool wait){
	if (wait) {
		int time = (int)std::ceil(data * 0.0065f * 400);
		Script::waitAndPrintProgress(time);
	}
	BUFFER_CLEAR
	status = viRead(instr, buffer, sizeof(buffer), &retCount);
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	std::string outV(reinterpret_cast<char*>(buffer), std::strlen(reinterpret_cast<char*>(buffer)));
	bool flipper = false;
	int cntr = 1;
	for (auto iter = outV.find(','); iter != std::string::npos; iter = outV.find(',')) {
		if (!flipper) {
			outV.replace(iter, 1, " ");
		}
		else {
			outV.replace(iter, 1, "\n");
		}
		flipper = !flipper;
		cntr++;
	}
	outputFileName << outV;
	std::cout << "Read: " << cntr << " datapoints" << std::endl;
}

ViStatus Keithley237::setFlag(const char* flag) {
	return status = viWrite(instr, (ViBuf)flag, (ViUInt32)strlen(flag), &writeCount);
}

void Keithley237::displayStatus() {
	std::cout << "Status: " << status << std::endl;
	std::cout << "Binary Status " << std::bitset<8>(status) << std::endl;
}