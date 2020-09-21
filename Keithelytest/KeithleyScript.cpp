#include "KeithleyScript.h"

void Script::displayHelp() {
	std::cout << "\nList of available functions:\n\n"
<< LABEL_KSCRIPT_DISPLAYHELP << " - display help message\n"
<< LABEL_KSCRIPT_WRITETODEVICE << " - write message to device directly (refer to device manual)\n"
<< LABEL_KSCRIPT_PULSE1 << " - 'old' pulsed mode\n"
<< LABEL_KSCRIPT_PULSE2 << " - pulsed mode for 50x100 new crossbars\n\
test2 - IV sweep (designed for big devices)\n\
testspecial - IV + stepped IV (designed for big devices)\n\
testsmart\n\
test0neg\n\
test0pos\n\
test0pulse\n"
<< LABEL_KSCRIPT_IT << "- I(t) measurement\n\
forming\n\
sdforming\n\
sdp\n\
sdppf\n\
ivheat - IV sweep for oven (designed for small devices)\n\
Multiv - multiple low Iv's\n\
IV -NEW PROPER IV FUNC\n\
exit - exit program" << std::endl;
}

void Script::IV_Script1(Keithley237& keithley) {
	std::cout << "Compliance? XE-Y\n>> ";
	char currentCompliance[10];
	std::cin >> currentCompliance;

	std::cout << "Forming compliance? XE-Y\n>> ";
	char FormingCompliance[10];
	std::cin >> FormingCompliance;

	std::cout << "Negative up to? (default -3) \n>> ";
	char negative_limit[10];
	std::cin >> negative_limit;
	double Fnegative_limit = strtof(negative_limit, nullptr);
	strcat_s(negative_limit, ",");
	char str2[30] = "Q1,0,";
	strcat_s(str2, negative_limit);

	std::cout << "Voltage step? (default 0.1) \n>> ";
	char voltage_step[20];
	std::cin >> voltage_step;
	strcat_s(str2, voltage_step);
	strcat_s(str2, ",0,");
	double Fvoltage_Step = strtof(voltage_step, nullptr);

	std::cout << "TimeStep? (default 400) \n>> ";
	char timeStep[10];
	std::cin >> timeStep;
	float FtimeStep = strtof(timeStep, nullptr);
	strcat_s(str2, timeStep);
	strcat_s(str2, "X");

	std::cout << "Positive up to? (default 4) \n>> ";
	char positive_limit[10];
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
	//	strcat_s(str5, ",0.3,1.5,0,");
	strcat_s(str5, timeStep);
	strcat_s(str5, "X");

	std::cout << "Amount of runs?\n>> ";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	keithley.setFlag(INTEGRATION_TIME_FAST);
	keithley.setFlag(FILTER_DISABLE);
	keithley.setFlag(OPERATE_OFF);
	keithley.setFlag(TRIGGER_DISABLE);
	keithley.set_dc_bias(0.0f, 0);
	keithley.setFlag(MODE_SWEEP);
	//	status = viWrite(instr, (ViBuf)"M2,X", (ViUInt32)strlen("M2,X"), &writeCount);	//Mask to get end of sweep
	keithley.setFlag(DATA_FORMAT_OUTPUT_SWEEP);
	keithley.setFlag(TRIGGER_ENABLE);
	keithley.setFlag(OPERATE_ON);

//	std::ofstream outputFailuresFileName("KeithOUTFailures.txt", std::ios::app);
	for (int i = 0; i < amountOfRuns; ++i) {
		std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
		//	Set sequence
		keithley.setCurrentCompliance(currentCompliance, 0);
		keithley.writeToDevice(str2);
		keithley.writeToDevice(str3);
		keithley.setFlag(TRIGGER_ACTION);
		keithley.readFromInstrument((int)std::ceil(Fnegative_limit / Fvoltage_Step) * -2 -1 + 0 + 4, true, 0.0014f * FtimeStep, false);	// (-3 * -20 + 1)
		//	Reset sequence
		keithley.setCurrentCompliance("1E-1", 0);
		keithley.writeToDevice(str4);
		keithley.writeToDevice(str5);
		keithley.setFlag(TRIGGER_ACTION);
		keithley.readFromInstrument((int)std::ceil(Fpositive_limit / Fvoltage_Step) * 2 + 0, true, 0.0014f * FtimeStep, false);
		//	if (!keithley.readFromInstrument((int)std::ceil(Fpositive_limit / Fvoltage_Step) * 2 + 0, true, 0.0015f * FtimeStep, true)) {
		//		outputFailuresFileName << i << std::endl;
		//		std::this_thread::sleep_for(std::chrono::seconds(10));
		//		std::cout << " - - - - - - Extra forming - - - - - - " << std::endl;
		//		keithley.setFlag(MODE_DC);
		//		keithley.setCurrentCompliance(FormingCompliance, 0);
		//		keithley.set_dc_bias(4.0f, 0);
		//		keithley.setFlag(TRIGGER_ACTION);
		//		waitAndPrintProgress(30);
		//		keithley.set_dc_bias(0.0f, 0);
		//		keithley.setFlag(MODE_SWEEP);
		//		std::cout << " - - - - - - Extra forming complete - - - - - - \n";
				//	status = viWrite(instr, (ViBuf)"Q1,0,-3,0.1,0,400X", (ViUInt32)strlen("Q1,0,-3,0.1,0,400X"), &writeCount);
				//	status = viWrite(instr, (ViBuf)"Q7,-2.9,0,1,0,400X", (ViUInt32)strlen("Q7,0,-2.9,1,0,400X"), &writeCount);
				//	TRIGGER_ACTION
				//	//	readFromInstrument(-3 * -10 + 1 + 4, true, 0.6f, false);	// (-3 * -20 + 1)
				//	std::this_thread::sleep_for(std::chrono::seconds(70));
				//	setCurrentCompliance("1E-1", 0);
				//	status = viWrite(instr, (ViBuf)"Q1,0.1,8,0.1,0,400X", (ViUInt32)strlen("Q1,0.1,8,0.1,0,400X"), &writeCount);
				//	status = viWrite(instr, (ViBuf)"Q7,7.9,0.3,1.5,0,400X", (ViUInt32)strlen("Q7,7.9,0.3,1.5,0,400X"), &writeCount);
				//	TRIGGER_ACTION
				//	//	readFromInstrument(4 * 10 + 4, true, 0.6f, false);	// (4 * 20)
				//	std::this_thread::sleep_for(std::chrono::seconds(70));

		//	}
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void Script::pulse_Script1(Keithley237& keithley) {
	std::cout << "Amount of runs?\n>> ";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	std::ofstream readsFileName("reads.txt", std::ios::app);
	std::ofstream assist_forming_counter_file("afc.txt", std::ios::app);
	float failTestNum;
	static std::string temp1;
	static std::string temp2;
	static std::string temp3;
	std::string result;
	std::string::size_type sz;

	auto analyze = [&]() {result = keithley.readInstrumentBuffer(outputFileName); temp3.assign(result, 12, 12);
	temp1.assign(result, 13, 7); temp2.assign(result, 22, 2);// std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
	failTestNum = std::stof(temp1, &sz) * (float)pow(10, -1 * std::stoi(temp2, &sz)); };

	bool get_out = false;
	int position;
	keithley.setCurrentCompliance("4E-4", 7);	//IMPORTANT if not here,the instrument starts saying PULSE TIME NOT MET
	char cc_cur[10];
	std::cout << "Compliance value? (default 4E-4)\n>> ";
	std::cin >> cc_cur;
	bool failcheckenable;
	std::cout << "Enable failcheck? (1/0)\n>> ";
	std::cin >> failcheckenable;
	keithley.set_dc_bias(0.0f, 0);
	keithley.setFlag(INTEGRATION_TIME_FAST);
	keithley.setFlag(FILTER_DISABLE);
	keithley.setFlag(OPERATE_OFF);
	keithley.setFlag(TRIGGER_DISABLE);
	keithley.setFlag(MODE_SWEEP);
	keithley.setFlag(DATA_FORMAT_OUTPUT_SWEEP_IV);
	keithley.setFlag(OPERATE_ON);
//	currentTime();
	keithley.setFlag(TRIGGER_ENABLE);

	for (int i = 0; i < amountOfRuns; ++i) {
		keithley.setCurrentCompliance(cc_cur, 7);
		//	status = viWrite(instr, (ViBuf)"Q3,5,2,1,500,100X", (ViUInt32)strlen("Q3,5,2,1,100,100X"), &writeCount);
		keithley.writeToDevice("Q3,-3,2,1,100,100X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

		//read part 1
		keithley.readInstrumentBuffer(outputFileName);
		//read part 2
		keithley.setCurrentCompliance("1E-3", 7);
		keithley.writeToDevice("Q3,0.05,2,1,500,500X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		analyze();
		std::cout << "Read1 current: " << failTestNum * (float)pow(10, 6) << " uAmps\n";

		if (failTestNum <= 0.00000079f && failcheckenable) {	//Less than 0.8uA
			while (!get_out) {
				std::cout << "Read1 threshold failed\n";
			//	pulses_failure_assistSmart();
				keithley.setFlag(MODE_SWEEP);
				keithley.setFlag(OPERATE_ON);
				assist_forming_counter_file << i + 1 << " ";
				keithley.setCurrentCompliance(cc_cur, 7);
				//		status = viWrite(instr, (ViBuf)"Q3,5,2,1,500,100X", (ViUInt32)strlen("Q3,5,2,1,100,100X"), &writeCount);
				keithley.writeToDevice("Q3,-4,2,1,500,500X");
				keithley.setFlag(TRIGGER_ACTION);
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				//read part 1
				keithley.readInstrumentBuffer(outputFileName);
				//read part 2
				keithley.setCurrentCompliance("1E-3", 7);
				keithley.writeToDevice("Q3,0.05,2,1,500,500X");
				keithley.setFlag(TRIGGER_ACTION);
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				analyze();
				std::cout << "Read1 current: " << failTestNum * (float)pow(10, 6) << " uAmps\n";

				if (failTestNum >= 0.00000099f) {
					get_out = true;
					std::cout << "Read1 threshold passed with help of -4V\n";
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

		keithley.setCurrentCompliance("1E-2", 9);
		keithley.writeToDevice("Q3,3.5,2,1,100,1000X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		//std::this_thread::sleep_for(std::chrono::milliseconds(200));

		//read part 1
		keithley.readInstrumentBuffer(outputFileName);
		std::this_thread::sleep_for(std::chrono::seconds(1));

		//read part 2
		keithley.setCurrentCompliance("1E-6", 4);
		keithley.writeToDevice("Q3,0.05,2,1,500,500X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		//	std::this_thread::sleep_for(std::chrono::milliseconds(300));
		analyze();
		std::cout << "Read2 current: " << failTestNum * (float)pow(10, 9) << " nAmps\n";

		if (failTestNum >= 0.000000199f) {	//More than 0.2uA
			while (!get_out) {
				std::cout << "Read2 threshold failed\n";
				keithley.setCurrentCompliance("1E-2", 9);
				keithley.writeToDevice("Q3,4,2,1,200,1000X");
				keithley.setFlag(TRIGGER_ACTION);
				std::cout << "- - - - - - - Applied +4V RESET- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(500));
					//read part 1
				keithley.readInstrumentBuffer(outputFileName);
				//read part 2
				keithley.setCurrentCompliance("1E-6", 4);
				keithley.writeToDevice("Q3,0.05,2,1,500,500X");
				keithley.setFlag(TRIGGER_ACTION);
				std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(300));
				analyze();
				std::cout << "Read2 current after +4V RESET: " << failTestNum * (float)pow(10, 9) << " nAmps\n";

				if (failTestNum <= 0.00000019f) {
					get_out = true;
					std::cout << "Read2 threshold passed with help1\n";
				}
				else {	//help2
					std::cout << "Read2 threshold failed\n";
					keithley.setCurrentCompliance("1E-2", 9);
					keithley.writeToDevice("Q3,5,2,1,1000,200X");
					keithley.setFlag(TRIGGER_ACTION);
					std::cout << "- - - - - - - Applied +5V RESET- - - - - -\n\n";
					std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(600));

					//read part 1
					keithley.readInstrumentBuffer(outputFileName);
					//read part 2
					keithley.setCurrentCompliance("1E-6", 4);
					keithley.writeToDevice("Q3,0.05,2,1,500,500X");
					keithley.setFlag(TRIGGER_ACTION);
					std::this_thread::sleep_for(std::chrono::seconds(3));
					//	std::this_thread::sleep_for(std::chrono::milliseconds(300));
					analyze();
					std::cout << "Read2 current after +5V RESET: " << failTestNum * (float)pow(10, 9) << " nAmps\n";

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

void Script::pulse_Script2(Keithley237& keithley) {
	std::cout << "Amount of runs?\n>> ";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	std::ofstream outputFileName("KeithOUT.txt", std::ios::app);
	std::ofstream readsFileName("reads.txt", std::ios::app);
	std::ofstream assist_forming_counter_file("afc.txt", std::ios::app);
	float failTestNum;
	static std::string temp1;
	static std::string temp2;
	static std::string temp3;
	std::string result;
	std::string::size_type sz;

	auto analyze = [&]() {result = keithley.readInstrumentBuffer(outputFileName); temp3.assign(result, 12, 12);
	temp1.assign(result, 13, 7); temp2.assign(result, 22, 2);// std::cout << "temp1: " << temp1 << " temp2: " << temp2 << std::endl;
	failTestNum = std::stof(temp1, &sz) * (float)pow(10, -1 * std::stoi(temp2, &sz)); };

	bool get_out = false;
	int position;
	keithley.setCurrentCompliance("4E-4", 7);	//IMPORTANT if not here,the instrument starts saying PULSE TIME NOT MET
	char cc_cur[10];
	std::cout << "Compliance value? (default 5E-6)\n>> ";
	std::cin >> cc_cur;
	bool failcheckenable;
	std::cout << "Enable failcheck? (1/0)\n>> ";
	std::cin >> failcheckenable;
	keithley.set_dc_bias(0.0f, 0);
	keithley.setFlag(INTEGRATION_TIME_FAST);
	keithley.setFlag(FILTER_DISABLE);
	keithley.setFlag(OPERATE_OFF);
	keithley.setFlag(TRIGGER_DISABLE);
	keithley.setFlag(MODE_SWEEP);
	keithley.setFlag(DATA_FORMAT_OUTPUT_SWEEP_IV);
	keithley.setFlag(OPERATE_ON);
	//	currentTime();
	keithley.setFlag(TRIGGER_ENABLE);

	for (int i = 0; i < amountOfRuns; ++i) {
		keithley.setCurrentCompliance(cc_cur, 7);
		//	status = viWrite(instr, (ViBuf)"Q3,5,2,1,500,100X", (ViUInt32)strlen("Q3,5,2,1,100,100X"), &writeCount);
		keithley.writeToDevice("Q3,-1,2,1,100,100X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		//	std::this_thread::sleep_for(std::chrono::milliseconds(300));

		//read part 1
		keithley.readInstrumentBuffer(outputFileName);
		//read part 2
		keithley.setCurrentCompliance("3E-4", 7);
		keithley.writeToDevice("Q3,0.05,2,1,500,500X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		analyze();
		std::cout << "Read1 current: " << failTestNum * (float)pow(10, 6) << " uAmps\n";

		if (failTestNum <= 0.000001f && failcheckenable) {	//Less than 1.0uA
			while (!get_out) {
				std::cout << "Read1 threshold failed\n";
				//	pulses_failure_assistSmart();
				keithley.setFlag(MODE_SWEEP);
				keithley.setFlag(OPERATE_ON);
				assist_forming_counter_file << i + 1 << " ";
				keithley.setCurrentCompliance(cc_cur, 7);
				//		status = viWrite(instr, (ViBuf)"Q3,5,2,1,500,100X", (ViUInt32)strlen("Q3,5,2,1,100,100X"), &writeCount);
				keithley.writeToDevice("Q3,-1.2,2,1,2000,2000X");
				keithley.setFlag(TRIGGER_ACTION);
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				//read part 1
				keithley.readInstrumentBuffer(outputFileName);
				//read part 2
				keithley.setCurrentCompliance("3E-4", 7);
				keithley.writeToDevice("Q3,0.05,2,1,500,500X");
				keithley.setFlag(TRIGGER_ACTION);
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::this_thread::sleep_for(std::chrono::milliseconds(300));

				analyze();
				std::cout << "Read1 current: " << failTestNum * (float)pow(10, 6) << " uAmps\n";

				if (failTestNum >= 0.000001f) {
					get_out = true;
					std::cout << "Read1 threshold passed with help of -1.2V\n";
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

		keithley.setCurrentCompliance("3E-4", 7);
		keithley.writeToDevice("Q3,2,2,1,300,1000X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		//std::this_thread::sleep_for(std::chrono::milliseconds(200));

		//read part 1
		keithley.readInstrumentBuffer(outputFileName);
		std::this_thread::sleep_for(std::chrono::seconds(1));

		//read part 2
		keithley.setCurrentCompliance("1E-6", 4);
		keithley.writeToDevice("Q3,0.05,2,1,500,500X");
		keithley.setFlag(TRIGGER_ACTION);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		//	std::this_thread::sleep_for(std::chrono::milliseconds(300));
		analyze();
		std::cout << "Read2 current: " << failTestNum * (float)pow(10, 9) << " nAmps\n";

		if (failTestNum >= 0.000000709f) {	//More than 0.7uA
			while (!get_out) {
				std::cout << "Read2 threshold failed\n";
				keithley.setCurrentCompliance("3E-4", 7);
				keithley.writeToDevice("Q3,3,2,1,200,1000X");
				keithley.setFlag(TRIGGER_ACTION);
				std::cout << "- - - - - - - Applied +3V RESET- - - - - -\n\n";
				std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(500));
					//read part 1
				keithley.readInstrumentBuffer(outputFileName);
				//read part 2
				keithley.setCurrentCompliance("1E-6", 4);
				keithley.writeToDevice("Q3,0.05,2,1,500,500X");
				keithley.setFlag(TRIGGER_ACTION);
				std::this_thread::sleep_for(std::chrono::seconds(3));
				//	std::this_thread::sleep_for(std::chrono::milliseconds(300));
				analyze();
				std::cout << "Read2 current after +3V RESET: " << failTestNum * (float)pow(10, 9) << " nAmps\n";

				if (failTestNum <= 0.000000709f) {
					get_out = true;
					std::cout << "Read2 threshold passed with help of +3V\n";
				}
			}
			get_out = false;
		}
		readsFileName << "\t" << temp3 << std::endl;
		assist_forming_counter_file << std::endl;
		outputFileName.flush();
		std::cout << " - - - - - - Run #" << i + 1 << " out of " << amountOfRuns << " complete - - - - - - \n\n";
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void Script::It_Script(Keithley237& keithley) {
	std::cout << "I(t) measurement mode\n";
	std::cout << "Voltage?\n>> ";
	char ItVoltage[10];
	std::cin >> ItVoltage;
	char str1[20] = "B";
	char str2[6] = ",0,0X";
	strcat_s(str1, ItVoltage);
	strcat_s(str1, str2);
	std::cout << "Compliance? XE-Y\n>> ";
	char currentCompliance[10];
	std::cin >> currentCompliance;
	char str3[20] = "L";
	char str4[4] = ",0X";
	strcat_s(str3, currentCompliance);
	strcat_s(str3, str4);
	std::cout << "tick time? (measurement per x seconds)\n>> ";
	int ticktime;
	std::cin >> ticktime;

	std::cout << "Apply filter? (1/0)\n>> ";
	bool filterstate;
	std::cin >> filterstate;
	if (filterstate == true) {
		keithley.setFlag(INTEGRATION_TIME_LINECYCLE60HZ);
			keithley.setFlag(FILTER_32READINGS);
			std::cout << "filtering is active" << std::endl;
	}
	else {
		keithley.setFlag(FILTER_DISABLE);
			keithley.setFlag(INTEGRATION_TIME_FAST);
	}
	std::ofstream outputFileName("KeithIt.txt", std::ios::app);
	keithley.setFlag(OPERATE_OFF);
	keithley.setFlag(TRIGGER_DISABLE);
	keithley.setFlag(MODE_DC);
	keithley.setFlag(DATA_FORMAT_OUTPUT_TICK);
	keithley.writeToDevice(str1);
	keithley.writeToDevice(str3);	//CC Settings
	keithley.setFlag(TRIGGER_ENABLE);
	keithley.setFlag(OPERATE_ON);
	keithley.setFlag(TRIGGER_ACTION);
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//	currentTime();
	std::cout << "I(t) experiment is going on" << std::endl;
	while (1) {
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> duration = now - start;
		float measTime = duration.count();
		outputFileName << measTime << "\t";
		keithley.readImmediateCurrentValue(outputFileName);
		outputFileName << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(ticktime));
	}
	keithley.setFlag(OPERATE_OFF);
}

void Script::multipleLowVoltageIVs(Keithley237& keithley) {
	std::cout << "at the moment this script does one go 0V-> -0.5V; 0V-> 0.5V" << std::endl;
	std::cout << "at the moment this script does one go 0V-> -0.5V; 0V-> 0.5V" << std::endl;
	std::cout << "at the moment this script does one go 0V-> -0.5V; 0V-> 0.5V" << std::endl;
	char currentCompliance[10] = "5E-3";
	std::string str2 = "Q2,-0.01,-0.1,3,0,400X";
	std::string str4 = "Q2,0.01,0.1,3,0,400X";

	std::string str3 = "Q8,-0.1,-0.01,3,0,400X";
	std::string str5 = "Q8,0.1,0.01,3,0,400X";
	int IVedge = 1;
	std::cout << "Amount of runs?\n";
	int amountOfRuns;
	std::cin >> amountOfRuns;
	keithley.setFlag(INTEGRATION_TIME_LINECYCLE60HZ);
	keithley.setFlag(FILTER_16READINGS);
	keithley.setFlag(OPERATE_OFF);
	keithley.setFlag(TRIGGER_DISABLE);
	keithley.set_dc_bias(0.0f, 0);
	keithley.setFlag(MODE_SWEEP);
	keithley.setFlag(DATA_FORMAT_OUTPUT_SWEEP_IV);	//read [source, measurement]
	keithley.setFlag(TRIGGER_ENABLE);
	int data[8] = { 60,70,80,90,100,110,120,130 };
	keithley.setCurrentCompliance(currentCompliance, 0);
	keithley.setFlag(OPERATE_ON);
//	currentTime();
	//	for (int i = 0; i < amountOfRuns; ++i) {
	int i = 4;
	IVedge = 5;
	str2.replace(12, 1, std::to_string(IVedge));
	str4.replace(10, 1, std::to_string(IVedge));
	str3.replace(6, 1, std::to_string(IVedge));
	str5.replace(5, 1, std::to_string(IVedge));
	std::cout << " - - - - - - Run #" << i + 1 << " of " << amountOfRuns << " - - - - - - " << std::endl;
	keithley.writeToDevice(str2.c_str());
	//	writeToDevice(str3.c_str());
	keithley.setFlag(TRIGGER_ACTION);
	keithley.readSweepFromInstrument(data[i], true);
	keithley.writeToDevice(str4.c_str());
	//	writeToDevice(str5.c_str());
	keithley.setFlag(TRIGGER_ACTION);
	keithley.readSweepFromInstrument(data[i], true);
	IVedge++;
	str2.replace(12, 1, std::to_string(IVedge));
	str4.replace(10, 1, std::to_string(IVedge));
	str3.replace(6, 1, std::to_string(IVedge));
	str5.replace(5, 1, std::to_string(IVedge));
	//	}
	std::cout << " - - - - - - Measurement complete - - - - - - \n";
}

void Script::playsong(Keithley237& keithley) {
	keithley.writeToDevice("D1,NUJNOnBOLSHEX");
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	keithley.writeToDevice("D1,DENEGX");
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	keithley.writeToDevice("D1,DENEGnPROSTOnTAKX");
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	keithley.writeToDevice("D1,PUSTnZAnNASX");
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	keithley.writeToDevice("D1,RABOTAETX");
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	keithley.writeToDevice("D1,TALANTLIVIYX");
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	keithley.writeToDevice("D1,INDUSX");
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	keithley.writeToDevice("D0X");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}

void Script::printProgress(float percentage) {
	std::string str = "............................................................";
	int value = (int)(percentage * 100);
	int progressLength = (int)(percentage * str.length());
	std::cout << "\r" << std::setw(3) << value << "% [" << std::setw(60) << std::left << str.substr(0, progressLength) << "]";
	std::cout.flush();
}

void Script::waitAndPrintProgress(int time) {
	for (int i = 0; i <= time; i++) {
		printProgress((float)i / (float)time);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << std::endl;
}