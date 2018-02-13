#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int fd,i;
	unsigned char axes,btns;
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
	ioctl(fd, JSIOCGCORR, cor);
	for (i=0; i<axes; i++) {
		printf("%d %d %d %d %d\n", cor[i].type,
					cor[i].coef[0], 
					cor[i].coef[1],
					cor[i].coef[2],
					cor[i].coef[3]);
	}
	return 0;
}
