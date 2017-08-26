#ifndef DAEMONIZER_HELPER
#define DAEMONIZER_HELPER

#include <string.h>

#include "../classes/Keyboard.h"

namespace daemonizer {
	
	void start(char *arg0, std::string scenarioFile = "");
	//void stop();
	//void stop(int signum);
	int setupEmit() ;
	void emit(int fd, int type, int code, int val);
	int closeEmit(int fd);
	void process(LedKeyboard &kbd, std::string scenarioFile = "");
	int start_macros(LedKeyboard &kbd);
	int process_macros(LedKeyboard &kbd);
	void setMNKey(hid_device *handle, LedKeyboard &kbd, std::string value);
	void setMRKey(hid_device *handle, LedKeyboard &kbd, std::string value);
}

#endif
