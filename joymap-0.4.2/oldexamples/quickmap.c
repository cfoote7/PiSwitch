#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include "keys.h"
#include "program.h"

//vendor=0x068e; //CH Products
//product=0x00f1; //PRO THROTTLE

//vendor=0x068e; //CH Products
//product=0x00f4; //COMBATSTICK

//vendor=0x068e; //CH Products
//product=0x00f2; //PEDALS

int keys[]={
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
};

int bang[MAX_SEQUENCE]={
	KEY_B,
	KEY_B|RELEASEMASK,
	KEY_A,
	KEY_A|RELEASEMASK,
	KEY_N,
	KEY_N|RELEASEMASK,
	KEY_G,
	KEY_G|RELEASEMASK,
	KEY_LEFTSHIFT,
	KEY_1,
	KEY_1|RELEASEMASK,
	KEY_LEFTSHIFT|RELEASEMASK,
	SEQUENCE_DONE,
};

int main(void) {
	int fd;
	int i;
	char devname[256];
	int vendor=0;
	int product=0;
	struct program_button_remap map;
	struct program_axis_remap amap;
	
	fd=open("/dev/mapper", O_RDWR);
	if (fd<0) {
		perror("Error programming joystick");
		return 0;
	}

	for (i=0; i<128; i++) {
		vendor=i;
		if (ioctl(fd, JOYMAP_VENDOR, &vendor)<0) {
			//perror("Failed to get vendor ID");
			continue;
		}
		product=i;
		if (ioctl(fd, JOYMAP_PRODUCT, &product)<0) {
			//perror("Failed to get product ID");
			continue;
		}
		sprintf(devname, " ");
		devname[0]=i;
		if (ioctl(fd, JOYMAP_NAME, devname)<0) {
			//perror("Failed to get product name");
			continue;
		}
		fprintf(stderr, "%d: Vendor=%x Product=%x [%s]\n", i, vendor, product, devname);	
	}

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f4; //COMBATSTICK
	map.srcbutton=BTN_JOYSTICK+0;
	map.device=DEVICE_KBD;
	map.type=TYPE_BUTTON;
	map.flags=FLAG_NONE;
	for (i=0; i<MAX_SEQUENCE; i++) {
		map.sequence[i]=bang[i];
	}
	write(fd, (char *)&map, sizeof(map));

	for (i=1; i<10; i++) {
		map.program=PROGRAM_BUTTON_REMAP;
		map.joystick=255;
		map.vendor=0x068e; //CH Products
		map.product=0x00f4; //COMBATSTICK
		map.srcbutton=BTN_JOYSTICK+i;
		map.device=DEVICE_KBD;
		map.type=TYPE_BUTTON;
		map.flags=FLAG_NONE;
		map.press=keys[i];
		map.sequence[1]=SEQUENCE_DONE;
		write(fd, (char *)&map, sizeof(map));
		map.program=PROGRAM_BUTTON_REMAP;
		map.joystick=255;
		map.vendor=0x068e; //CH Products
		map.product=0x00f4; //COMBATSTICK
		map.srcbutton=BTN_JOYSTICK+i;
		map.device=DEVICE_KBD;
		map.type=TYPE_BUTTON;
		map.flags=FLAG_RELEASE;
		map.press=keys[i]|RELEASEMASK;
		map.sequence[1]=SEQUENCE_DONE;
		write(fd, (char *)&map, sizeof(map));
	}

	amap.program=PROGRAM_AXIS_REMAP;
	amap.joystick=255;
	amap.vendor=0x068e; //CH Products
	amap.product=0x00f1; //PRO THROTTLE
	amap.srcaxis=ABS_X;
	amap.device=DEVICE_MOUSE;
	amap.type=TYPE_AXIS;
	amap.flags=FLAG_NONE;
	amap.plus=ABS_X;
	amap.minus=ABS_X;
	amap.axis=ABS_X;
	write(fd, (char *)&amap, sizeof(amap));

	amap.program=PROGRAM_AXIS_REMAP;
	amap.joystick=255;
	amap.vendor=0x068e; //CH Products
	amap.product=0x00f1; //PRO THROTTLE
	amap.srcaxis=ABS_Y;
	amap.device=DEVICE_MOUSE;
	amap.type=TYPE_AXIS;
	amap.flags=FLAG_NONE;
	amap.plus=ABS_Y;
	amap.minus=ABS_Y;
	amap.axis=ABS_Y;
	write(fd, (char *)&amap, sizeof(amap));

	//map four way switch to mouse
	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+0;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_AXIS;
	map.flags=FLAG_NONE;
	map.press=ABS_X;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+2;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_AXIS;
	map.flags=FLAG_INVERT;
	map.press=ABS_X;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+1;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_AXIS;
	map.flags=FLAG_NONE;
	map.press=ABS_Y;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+3;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_AXIS;
	map.flags=FLAG_INVERT;
	map.press=ABS_Y;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+7;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_AXIS;
	map.flags=FLAG_INVERT;
	map.press=ABS_WHEEL;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+5;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_AXIS;
	map.flags=FLAG_NONE;
	map.press=ABS_WHEEL;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+13;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_BUTTON;
	map.flags=FLAG_NONE;
	map.press=BTN_LEFT;
	map.sequence[1]=SEQUENCE_DONE;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+13;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_BUTTON;
	map.flags=FLAG_RELEASE;
	map.press=BTN_LEFT|RELEASEMASK;
	map.sequence[1]=SEQUENCE_DONE;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+15;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_BUTTON;
	map.flags=FLAG_NONE;
	map.press=BTN_RIGHT;
	map.sequence[1]=SEQUENCE_DONE;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+15;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_BUTTON;
	map.flags=FLAG_RELEASE;
	map.press=BTN_RIGHT|RELEASEMASK;
	map.sequence[1]=SEQUENCE_DONE;
	write(fd, (char *)&map, sizeof(map));


	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+14;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_BUTTON;
	map.flags=FLAG_NONE;
	map.press=BTN_MIDDLE;
	map.sequence[1]=SEQUENCE_DONE;
	write(fd, (char *)&map, sizeof(map));

	map.program=PROGRAM_BUTTON_REMAP;
	map.joystick=1;
	map.joystick=255;
	map.vendor=0x068e; //CH Products
	map.product=0x00f1; //PRO THROTTLE
	map.srcbutton=BTN_JOYSTICK+14;
	map.device=DEVICE_MOUSE;
	map.type=TYPE_BUTTON;
	map.flags=FLAG_RELEASE;
	map.press=BTN_MIDDLE|RELEASEMASK;
	map.sequence[1]=SEQUENCE_DONE;
	write(fd, (char *)&map, sizeof(map));

	ioctl(fd, JOYMAP_MAP);
	close(fd);
}

