#include "daemonizer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <iomanip>
#include <locale>
#include <fstream>
#include <sstream>
#include <string> 
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/uinput.h>
#include <fcntl.h>

#include "utils.h"
#include "vboard.h"
#include "../classes/Keyboard.h"

#define BLINK_DELAY 10000 // TODO: MAKE REAL TIME TESTED

std::array<std::array<std::array<int, 50>, 9>, 3> my_macros ;
std::vector<int> record_buff;
vboard *fake_board;

namespace daemonizer {
	
	hid_device *handle0, *handle1;
	
	void start(char *arg0, std::string scenarioFile) {
		pid_t pid, sid;
		
		pid = fork();
		if (pid < 0) exit(EXIT_FAILURE);
		if (pid > 0) exit(EXIT_SUCCESS);
		sid = setsid();
		if (sid < 0) exit(EXIT_FAILURE);
		signal(SIGCHLD,SIG_IGN); /* ignore child */
		signal(SIGHUP,SIG_IGN); /* ignore child */
		
		pid = fork();
		if (pid < 0) exit(EXIT_FAILURE);
		if (pid > 0) exit(EXIT_SUCCESS);
		umask(0);
		if (chdir("/") < 0) exit(EXIT_FAILURE);
		
		std::cout<<getpid()<<std::endl;
		
		//close(STDIN_FILENO);
		//close(STDOUT_FILENO);
		//close(STDERR_FILENO);
		
		signal(SIGCHLD,SIG_IGN); /* ignore child */
		signal(SIGHUP,SIG_IGN); /* ignore child */
		signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
		signal(SIGTTOU,SIG_IGN);
		signal(SIGTTIN,SIG_IGN);		
		//signal(SIGINT, stop);
		
		fake_board = new vboard();
		
		LedKeyboard kbd;
		kbd.open();
		std::cout<<"Started Daemon"<<std::endl;
		start_macros(kbd);
		
		bool do_file_proc = true;
		if (scenarioFile == "")
			do_file_proc = false;
		
		while(true){
			if (do_file_proc)
				process(kbd, scenarioFile);
			process_macros(kbd);
			usleep(1);
		}
	}
	/*
	void stop() { stop(2); }
	void stop(int signum) {
		// Set exit keyboard profile
		exit(signum);
	}
	*/
	
	void process(LedKeyboard &kbd, std::string scenarioFile) {
		static bool fileChanged = false;
		static std::string parsedFileString = "";
		static time_t scenaryFileLastUpdate = 0;
		
		if (! fileChanged) {
			struct stat file_stat;
			if (stat(scenarioFile.c_str(), &file_stat) == 0) {
				if(file_stat.st_mtime > scenaryFileLastUpdate) {
					fileChanged = true;
					scenaryFileLastUpdate = file_stat.st_mtime;
				}
			}
		}
		
		if (fileChanged) {
			fileChanged = false;
			std::ifstream file;
			file.open(scenarioFile);
			if (file.is_open()) {
				std::string line;
				parsedFileString = "";
				while (!file.eof()) {
					getline(file, line);
					if (line.substr(0, 2) == "p ") {
						std::ifstream profileFile;
						profileFile.open(line.substr(2));
						if (file.is_open()) {
							std::string profileLine;
							while (!profileFile.eof()) {
								getline(profileFile, profileLine);
								if (profileLine != "" && profileLine.substr(0,1) != "#") parsedFileString = parsedFileString + profileLine + "\n";
							}
							profileFile.close();
						}
					} else {
						if (line != "" && line.substr(0,1) != "#") parsedFileString = parsedFileString + line + "\n";
					}
				}
				file.close();
			}
		}
		
		std::istringstream parsedFile(parsedFileString);
		utils::parseProfile(kbd, parsedFile);		
	}
	
	int start_macros(LedKeyboard &kbd) {
		struct hid_device_info *devs, *cur_dev, *dev0 = NULL, *dev1 = NULL;
		
		if (hid_init())
			return -1;
		
		devs = hid_enumerate(0x046d, 0xc335);
		cur_dev = devs;
		while(cur_dev) {
			if(cur_dev->interface_number == 0) {
				dev0 = cur_dev;
			}
			else if(cur_dev->interface_number == 1) {
				dev1 = cur_dev;
			}
			cur_dev = cur_dev->next;
		}
		
		handle0 = hid_open_path(dev0->path);
		handle1 = hid_open_path(dev1->path);
		if (!handle0 || !handle1) {
			std::cout<<"unable to open device\n"<<std::endl;
			return 1;
		}
		hid_free_enumeration(devs);
		
		hid_set_nonblocking(handle0, 1);
		hid_set_nonblocking(handle1, 1);
		
		utils::setGKeysMode(kbd, "1");
		setMNKey(handle1, kbd, "1");
		setMRKey(handle1, kbd, "0");
		
		return 0;
	}
	
	int process_macros(LedKeyboard &kbd) {
		static unsigned char buf0[256], buf1[256];
		static int res0, res1;
		static char blink_state = 0, record_macro = 0, last_m_keys_pressed = 0, current_m_key = 1;
		static unsigned long blink_count;
		static unsigned char runCounts = 0;
		// TODO: Make each profile have it's own macros.
		// TODO: Store profiles
		res0 = hid_read(handle0, buf0, sizeof(buf0));
		res1 = hid_read(handle1, buf1, sizeof(buf1));	

		if (record_macro > 0 && res0 > 0) {
			record_buff.push_back(buf0[2]);
		}
		else if (res0 < 0) {
			std::cout<<"hid_read() returned error\n"<<std::endl;
		}
		
		if (res1 > 0) {
			if (buf1[2] == 0x08 && (buf1[4] || buf1[5])) {
				if (record_macro == -1) {
					int buf_tmp = buf1[5] * 256 + buf1[4];
					for (int i = 0; i < 9; i++) {
						if (!((buf_tmp >> i) ^ 1)) {
							record_macro = i + 1;
							blink_count = 0;
							blink_state = 1;
							break;
						}
					}
				}
				else if (record_macro > 0) {
					record_buff.clear();
					record_macro = 0;
					setMRKey(handle1, kbd, "0");
				}
				else {
					int buf_tmp = buf1[5] * 256 + buf1[4];
					int i_tmp = 0;
					for (int i = 0; i < 9; i++) {
						if (!((buf_tmp >> i) ^ 1)) {
							i_tmp = i + 1;
							blink_count = 0;
							blink_state = 1;
							break;
						}
					}
					for (int i = 0; i < 50; i++)
					{
						fake_board->print(my_macros[current_m_key - 1][i_tmp][i]);
						printf("iprinting %c\n", my_macros[current_m_key - 1][i_tmp][i] + 61);
					}
					std::cout<<std::endl;
					//macro_play(current_m_key, buf_tmp >> i);
				}
			}
			else if (buf1[2] == 0x09) {
				if (record_macro) {
					record_buff.clear();
					record_macro = 0;
					setMRKey(handle1, kbd, "0");
				}
				if (!last_m_keys_pressed != !buf1[4]) {
					if (buf1[4] == 1 || buf1[4] == 2 || buf1[4] == 4) {
						int buf_tmp = buf1[4];
						int i_tmp = 0;
						for (int i = 0; i < 3; i++) {
							if (!((buf_tmp >> i) ^ 1)) {
								i_tmp = i + 1;
								blink_count = 0;
								blink_state = 1;
								break;
							}
						}
						current_m_key = i_tmp;
						setMNKey(handle1, kbd, std::to_string(buf1[4]));
					}
					last_m_keys_pressed = buf1[4];
				}
			}
			else if ((buf1[2] == 0x0a) && buf1[4])
			{
				if (!record_macro) {
					record_macro = -1;
					setMRKey(handle1, kbd, "1");
				}
				else if (record_macro > 0) {
					std::cout<<std::to_string(record_macro)<< std::endl;
					
					int i_tmp = 0;
					for (int i = 0; i < 3; i++) {
						if (!((buf1[4] >> i) ^ 1)) {
							i_tmp = i;
							break;
						}
					}
					for (unsigned int j = 0; j < record_buff.size(); j++) {
						my_macros[i_tmp][record_macro][j] = record_buff[j];
						printf("got %d\n", record_buff[j]);
					}
					record_macro = 0;
					setMRKey(handle1, kbd, "0");
				}
				else {
					record_buff.clear();
					record_macro = 0;
					setMRKey(handle1, kbd, "0");
				}
			}
		}
		else if (res1 < 0) {
			std::cout<<"hid_read() returned error\n"<<std::endl;
		}
		if (record_macro > 0) {
			if (blink_count == 0 ) {
				blink_count = BLINK_DELAY;
				blink_state = blink_state ? 0 : 1;
				setMRKey(handle1, kbd, std::to_string(blink_state));
			}
			else {
				blink_count --;
			}
		}
		
		if (runCounts < 256) {	// Init messes with keys scanning. This just makes sure everything's in the right state so that the first M-key press isn't missed.
			runCounts++;
			last_m_keys_pressed = 0;
			current_m_key = 1;
		}
		return 0;
	}
	
	void setMNKey(hid_device *handle, LedKeyboard &kbd, std::string value) {
		static unsigned char buf[256];
		utils::setMNKey(kbd, value);
		hid_set_nonblocking(handle, 0);
		hid_read(handle, buf, sizeof(buf));	
		hid_set_nonblocking(handle, 1);
	}
	
	void setMRKey(hid_device *handle, LedKeyboard &kbd, std::string value) {
		static unsigned char buf[256];
		utils::setMRKey(kbd, value);
		hid_set_nonblocking(handle, 0);
		hid_read(handle, buf, sizeof(buf));	
		hid_set_nonblocking(handle, 1);
	}
}

