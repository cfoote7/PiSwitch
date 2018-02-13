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
	int min[256];
	int max[256];
	int dmin[256];
	int dmax[256];
	int count=0;
	int nobutton=0;
	unsigned char axes, btns;
	struct js_event ev;
	struct js_corr cor[64];
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
	for (i=0; i<64; i++) {
		cor[i].type=JS_CORR_NONE;
		cor[i].coef[0]=128;
		cor[i].coef[1]=128;
		cor[i].coef[2]=65536*64;
		cor[i].coef[3]=65536*64;
	}
	ioctl(fd, JSIOCSCORR, cor);

	printf("Leave all axes centred, press a button when ready\n");
	print_data_hdr(aval, bval, axes, btns);

	for (i=0; i<axes; i++) aval[i]=128;
	for (i=0; i<axes; i++) bval[i]=0;
	//get initial values;
	nobutton=0;
	while (nobutton==0) {
		while (read(fd, &ev, sizeof(ev))==sizeof(ev)) {
			if (ev.type&JS_EVENT_BUTTON)
				bval[ev.number]=ev.value;
			if (ev.type&JS_EVENT_AXIS)
				aval[ev.number]=ev.value;
		}
		print_data(aval, bval, axes, btns);
		nobutton=0;
		for (i=0; i<btns; i++) nobutton+=bval[i];
	}
	//set initial deadzone
	for (i=0; i<axes; i++) dmin[i]=dmax[i]=aval[i];
	nobutton=0;
	
	//wait five seconds for scan of deadzone
	count=0;
	while (count<500) {
		while (read(fd, &ev, sizeof(ev))==sizeof(ev)) {
			if (ev.type&JS_EVENT_BUTTON)
				bval[ev.number]=ev.value;
			if (ev.type&JS_EVENT_AXIS) {
				aval[ev.number]=ev.value;
				if (aval[ev.number]<dmin[ev.number])
					dmin[ev.number]=aval[ev.number];
				if (aval[ev.number]>dmax[ev.number])
					dmax[ev.number]=aval[ev.number];
			}
		}
		print_data(aval, bval, axes, btns);
		usleep(1000);
		count++;
	}


	for (i=0; i<axes; i++) min[i]=max[i]=aval[i];
	printf("\n\n");
	printf("Move axes through complete range of motion.\n");
	printf("Press a button when complete\n");
	print_data_hdr(aval, bval, axes, btns);
	while (nobutton==0) {
		while (read(fd, &ev, sizeof(ev))==sizeof(ev)) {
			if (ev.type&JS_EVENT_BUTTON)
				bval[ev.number]=ev.value;
			if (ev.type&JS_EVENT_AXIS) {
				aval[ev.number]=ev.value;
				if (aval[ev.number]<min[ev.number])
					min[ev.number]=aval[ev.number];
				if (aval[ev.number]>max[ev.number])
					max[ev.number]=aval[ev.number];
			}
		}
		print_data(aval, bval, axes, btns);
		nobutton=0;
		for (i=0; i<btns; i++) nobutton+=bval[i];
	}
	printf("\n");
	for (i=0; i<axes; i++) {
		cor[i].type=JS_CORR_BROKEN;
		printf("%d: %d %d %d %d\n", i, dmin[i], dmax[i], min[i], max[i]);
		if (max[i]==dmax[i]) dmax[i]=max[i]-1;
		if (min[i]==dmin[i]) dmin[i]=min[i]+1;
		cor[i].coef[0]=min[i];
		cor[i].coef[1]=min[i];
		cor[i].coef[2]=255*16384/(max[i]-min[i]);
		cor[i].coef[3]=255*16384/(max[i]-min[i]);
	}
	ioctl(fd, JSIOCSCORR, cor);
	return 0;
}
