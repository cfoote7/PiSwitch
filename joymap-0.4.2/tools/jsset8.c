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
		min[i]=max[i]=0;
	}
	ioctl(fd, JSIOCSCORR, cor);

	while (read(fd, &ev, sizeof(ev))==sizeof(ev)) {
		if (ev.type&JS_EVENT_BUTTON)
			bval[ev.number]=ev.value;
		if (ev.type&JS_EVENT_AXIS) {
			aval[ev.number]=ev.value;
			if (ev.value<min[ev.number])
				min[ev.number]=ev.value;	
			if (ev.value>max[ev.number])
				max[ev.number]=ev.value;	
		}
	}
	usleep(100000);
	count=0;
	while (count++<100) {
		while (read(fd, &ev, sizeof(ev))==sizeof(ev)) {
			if (ev.type&JS_EVENT_BUTTON)
				bval[ev.number]=ev.value;
			if (ev.type&JS_EVENT_AXIS) {
				aval[ev.number]=ev.value;
				if (ev.value<min[ev.number])
					min[ev.number]=ev.value;	
				if (ev.value>max[ev.number])
					max[ev.number]=ev.value;	
			}
		}
		usleep(10000);
	}

	for (i=0; i<axes; i++) {
		cor[i].type=JS_CORR_BROKEN;
		cor[i].coef[0]=0;
		cor[i].coef[1]=0;
		cor[i].coef[2]=255*16384/255;
		cor[i].coef[3]=255*16384/255;
		if ((min[i]==max[i])&&(max[i]==0)) {
			//assume a switching hat
			cor[i].coef[0]=0;
			cor[i].coef[1]=0;
			cor[i].coef[2]=1*16384/1;
			cor[i].coef[3]=1*16384/1;
		}
	}
	ioctl(fd, JSIOCSCORR, cor);
	return 0;
}
