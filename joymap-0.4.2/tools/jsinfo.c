#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <stdlib.h>

void print_data(int aval[], int bval[], int axes, int btns) {
	int i;
	for (i=0; i<btns; i++) {
		printf("%2d |", bval[i]);
	}
	for (i=0; i<axes; i++) {
		printf(" %5d |", aval[i]);
	}
	printf("    \r");
	fflush(stdout);
}

void print_data_hdr(int aval[], int bval[], int axes, int btns) {
	int i;
	printf("js0: axes=%d, buttons=%d\n", axes, btns);
	for (i=0; i<btns; i++) {
		printf("b%02d|", i);
	}
	for (i=0; i<axes; i++) {
		printf("  a%02d  |", i, aval[i]);
	}
	printf("\n");
	for (i=0; i<btns; i++) {
		printf("____");
	}
	for (i=0; i<axes; i++) {
		printf("________");
	}
	printf("\n");
}

int main(int argc, char *argv[]) {
	int fd,i;
	int aval[256];
	int bval[256];
	unsigned char axes, btns;
	struct js_event ev;
	struct js_corr cor[8];
	if (argc>=2)
		fd=open(argv[1], O_RDONLY|O_NONBLOCK);
	else
		fd=open("/dev/input/js0", O_RDONLY|O_NONBLOCK);
	if (fd<0) {	
		perror("Failed to open device");
		return 1;
	}
	ioctl(fd, JSIOCGAXES, &axes);
	ioctl(fd, JSIOCGBUTTONS, &btns);

	print_data_hdr(aval, bval, axes, btns);

	for (i=0; i<100; i++) {
		if (read(fd, &ev, sizeof(ev))==sizeof(ev)) {
			if (ev.type&JS_EVENT_BUTTON)
				bval[ev.number]=ev.value;
			if (ev.type&JS_EVENT_AXIS)
				aval[ev.number]=ev.value;
		}
	}
	while (1) {
		if (read(fd, &ev, sizeof(ev))==sizeof(ev)) {
			if (ev.type&JS_EVENT_BUTTON)
				bval[ev.number]=ev.value;
			if (ev.type&JS_EVENT_AXIS)
				aval[ev.number]=ev.value;
		}
		print_data(aval, bval, axes, btns);
	}
	printf("\n");
	return 0;
}
