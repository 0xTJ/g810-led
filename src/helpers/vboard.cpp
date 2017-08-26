#include "vboard.h"

vboard::vboard() {
	struct uinput_setup usetup;

	vboard_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	ioctl(vboard_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(vboard_fd, UI_SET_KEYBIT, KEY_SPACE);

	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0xDEAD;
	usetup.id.product = 0xBEEF;
	strcpy(usetup.name, "Example device");

	ioctl(vboard_fd, UI_DEV_SETUP, &usetup);
	ioctl(vboard_fd, UI_DEV_CREATE);

	sleep(1);
}

void vboard::emit(int fd, int type, int code, int val) {
	struct input_event ie;

	ie.type = type;
	ie.code = code;
	ie.value = val;
	/* timestamp values below are ignored */
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	write(fd, &ie, sizeof(ie));
}

void vboard::print(int code) {
	emit(vboard_fd, EV_KEY, code, 1);
	emit(vboard_fd, EV_SYN, SYN_REPORT, 0);
	emit(vboard_fd, EV_KEY, code, 0);
	emit(vboard_fd, EV_SYN, SYN_REPORT, 0);
}

vboard::~vboard() {
	sleep(1);
	ioctl(vboard_fd, UI_DEV_DESTROY);
	close(vboard_fd);
}