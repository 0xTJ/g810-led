#ifndef VBOARD_H
#define VBOARD_H

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

#include <sys/ioctl.h>

class vboard {
public:
	int vboard_fd;
	
	vboard();
	void emit(int fd, int type, int code, int val);
	void print(int code);
	~vboard();
};

#endif
