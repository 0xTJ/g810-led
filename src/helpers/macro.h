#ifndef MACRO_HELPER
#define MACRO_HELPER

#include <iostream>
#include <string>
#include <vector>

#include "hidapi/hidapi.h"
using namespace std;

namespace macro {
	typedef std::vector<int> int_vec_t;
	//int_vec_t macros;
	void proc_mac_keys(unsigned char dev_code, unsigned char key_code,hid_device *handle);
}

#endif
