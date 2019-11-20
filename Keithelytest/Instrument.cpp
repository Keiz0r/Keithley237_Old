#include "Instrument.h"

ViStatus instrument::writeToInstrument(const char* msg)
{
	return status = viWrite(instr, (ViBuf)msg, (ViUInt32)strlen(msg), &writeCount);
}

void instrument::readSweepFromInstrument(int place, int length)
{
	int bytesToRead;
	writeToInstrument("U11X");
	status = viRead(instr, buffer, 100, &retCount);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	Sbuffer.assign(Sbuffer, 3, 4);
	bytesToRead = std::stoi(Sbuffer) * 13;
	status = viRead(instr, buffer, bytesToRead, &retCount);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}
	Sbuffer.assign(Sbuffer, place, length);
}

void instrument::readSweepFromInstrument()
{
	int bytesToRead;
	writeToInstrument("U11X");
	status = viRead(instr, buffer, 100, &retCount);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	Sbuffer.assign(Sbuffer, 3, 4);
	bytesToRead = std::stoi(Sbuffer) * 13;
	status = viRead(instr, buffer, bytesToRead, &retCount);
	Sbuffer = reinterpret_cast<char const*>(buffer);
	//Format data into column
	int position = Sbuffer.find(",");
	while (position != std::string::npos) {
		Sbuffer.replace(position, 1, "\n");
		position = Sbuffer.find(",", position + 1);
	}
	Sbuffer.assign(Sbuffer, 0, bytesToRead);
}
