#include "Instrument.h"

ViStatus instrument::writeToInstrument(ViSession instrID, const char* msg)
{
	return status = viWrite(instrID, (ViBuf)msg, (ViUInt32)strlen(msg), &writeCount);
}
