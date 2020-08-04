#define	_CRT_SECURE_NO_WARNINGS
#include "KeithleyScript.h"

static std::string userMessage;

int main()
{
	std::cout << "\n\
                              Keithley 237 automation protocol\n\
                                Build: " << __DATE__ << " " << __TIME__ << "\n\
                                      Pavel Baikov\n" << std::endl;
	Keithley237 kthley1("GPIB0::2::INSTR");
//	if (connectDevice()) Script::displayHelp();
	Script::displayHelp();	//This ruins other functions for now except IV

	while (1) {
		std::cout << ">> ";
		std::cin >> userMessage;

		if (userMessage == "status") {
			kthley1.displayStatus();
		}
		else if (userMessage == "connect") {
			kthley1.connect();
		}
		else if (userMessage == "disconnect") {
			kthley1.disconnect();
		}
		else if (userMessage == "write") {
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
		else if (userMessage == "pulse1") {
			Script::pulse_Script1(kthley1);
		}
		else if (userMessage == "disco") {
			Script::playsong(kthley1);
		}
	//	else if (userMessage == "it") {
	//		Itmeas();
	//	}
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
	//	else if (userMessage == "Multiv") {
	//		MultipleLowVoltageIvs();
	//	}
		else if (userMessage == "IV") {
			Script::IV_Script1(kthley1);
		}
		else if (userMessage == "help") {
			Script::displayHelp();
		}
		else if (userMessage == "exit") {
			break;
		}
		else {
			std::cout << "Wrong input\n";
		}
	}

	return 0;
}