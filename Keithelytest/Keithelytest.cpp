#define	_CRT_SECURE_NO_WARNINGS
#include "KeithleyScript.h"
#include <time.h>

static std::string userMessage;

void currentTime() {
	time_t rawtime;
	tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	std::cout << "Current local time and date: " << asctime(timeinfo) << std::endl;
}

int main()
{
	std::cout << "\n\
                              Keithley 237 automation protocol\n\
                                Build: " << __DATE__ << " " << __TIME__ << "\n\
                                      Pavel Baikov\n" << std::endl;
	Keithley237 kthley1("GPIB0::2::INSTR");

	while (1) {
		std::cout << ">> ";
		std::cin >> userMessage;

		if (userMessage == LABEL_KSCRIPT_STATUS) {
			kthley1.displayStatus();
		}
		else if (userMessage == LABEL_KSCRIPT_CONNECT) {
			kthley1.connect();
		}
		else if (userMessage == LABEL_KSCRIPT_DISCONNECT) {
			kthley1.disconnect();
		}
		else if (userMessage == LABEL_KSCRIPT_WRITETODEVICE) {
			std::cin >> userMessage;
			kthley1.writeToDevice(userMessage.c_str());
		}
	//	else if (userMessage == "readm") {
	//		readMessageFromDevice();
	//	}
	//	else if (userMessage == "test2") {
	//		IV_meas2();
	//	}
	//	else if (userMessage == "testspecial") {
	//		IV_plus_steppedIV();
	//	}
	//	else if (userMessage == "testsmart") {
	//		IV_measSmart();
	//	}
	//	else if (userMessage == "test0neg") {
	//		test0neg();
	//	}
	//	else if (userMessage == "test0pos") {
	//		test0pos();
	//	}
		else if (userMessage == LABEL_KSCRIPT_PULSE1) {
			Script::pulse_Script1(kthley1);
		}
		else if (userMessage == LABEL_KSCRIPT_PULSE2) {
			//new 50x100device set -1V, reset +2V
			Script::pulse_Script2(kthley1);
		}
		else if (userMessage == "disco") {
			Script::playsong(kthley1);
		}
		else if (userMessage == LABEL_KSCRIPT_IT) {
			Script::It_Script(kthley1);
		}
	//	else if (userMessage == "forming") {
	//		forming();
	//	}
	//	else if (userMessage == "sdforming") {
	//		smallDeviceForming();
	//	}
	//	else if (userMessage == "sdp") {
	//		small_device_pulsed_mode();
	//	}
	//	else if (userMessage == "sdppf") {
	//		small_device_pulsed_mode_proper_forming();
	//	}
	//	else if (userMessage == "ivheat") {
	//		IV_meas_thermal_Smart();
	//	}
		else if (userMessage == "Multiv") {
			Script::multipleLowVoltageIVs(kthley1);
		}
		else if (userMessage == "IV") {
			Script::IV_Script1(kthley1);
		}
		else if (userMessage == LABEL_KSCRIPT_DISPLAYHELP) {
			Script::displayHelp();
		}
		else if (userMessage == "exit") {
			break;
		}
		else {
			std::cout << "Wrong input\n";
		}
		currentTime();
	}

	return 0;
}