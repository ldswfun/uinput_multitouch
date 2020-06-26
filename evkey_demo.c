#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/uinput.h>
#include <linux/input.h>
 
 
#define KEY_CUSTOM_UP 0x20
#define KEY_CUSTOM_DOWN 0x30
 
 
static struct uinput_user_dev uinput_dev;
static int uinput_fd;
 
int creat_user_uinput(void);
int report_key(unsigned int type, unsigned int keycode, unsigned int value);
 
 
int main(int argc, char *argv[])
{
	char kbd_buf[32];
	int ret = 0;
	ret = creat_user_uinput();
	if(ret < 0){
		printf("%s:%d\n", __func__, __LINE__);
		return -1;//error process.
	}
	

	printf("input up/down:");

	while(1)
	{
		fgets(kbd_buf, 32, stdin);
		if(strncmp(kbd_buf, "up", 2) == 0)
		{
			
			report_key(EV_KEY, KEY_VOLUMEUP, 1);// Report BUTTON A CLICK - PRESS event
			report_key(EV_KEY, KEY_VOLUMEUP, 0);// Report BUTTON A CLICK - RELEASE event
		}else if(strncmp(kbd_buf, "down", 3) == 0)
		{
			report_key(EV_KEY, KEY_VOLUMEDOWN, 1);
			report_key(EV_KEY, KEY_VOLUMEDOWN, 0);
		}else{
			printf("unsupport key\n");
		}
	}	

	
	
	close(uinput_fd);
	return 0;
}
 
 
int creat_user_uinput(void)
{
	int i;
	int ret = 0;
 
	uinput_fd = open("/dev/uinput", O_RDWR | O_NDELAY);
	if(uinput_fd < 0){
		printf("%s:%d\n", __func__, __LINE__);
		return -1;//error process.
	}
	
	//to set uinput dev
	memset(&uinput_dev, 0, sizeof(struct uinput_user_dev));
	snprintf(uinput_dev.name, UINPUT_MAX_NAME_SIZE, "uinput-custom-dev");
	uinput_dev.id.version = 1;
	uinput_dev.id.bustype = BUS_VIRTUAL;
	
	ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);
	ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(uinput_fd, UI_SET_EVBIT, EV_MSC);
	
	for(i = 0; i < 256; i++){
		ioctl(uinput_fd, UI_SET_KEYBIT, i);
	}
	ioctl(uinput_fd, UI_SET_MSCBIT, KEY_CUSTOM_UP);
	ioctl(uinput_fd, UI_SET_MSCBIT, KEY_CUSTOM_DOWN);
	
	ret = write(uinput_fd, &uinput_dev, sizeof(struct uinput_user_dev));
	if(ret < 0){
		printf("%s:%d\n", __func__, __LINE__);
		return ret;//error process.
	}
	
	ret = ioctl(uinput_fd, UI_DEV_CREATE);
	if(ret < 0){
		printf("%s:%d\n", __func__, __LINE__);
		close(uinput_fd);
		return ret;//error process.
	}
	
	return 0;
}
 
int report_key(unsigned int type, unsigned int keycode, unsigned int value)
{
	struct input_event key_event;
	int ret;
	
	memset(&key_event, 0, sizeof(struct input_event));
	
	gettimeofday(&key_event.time, NULL);
	key_event.type = type;
	key_event.code = keycode;
	key_event.value = value;
	ret = write(uinput_fd, &key_event, sizeof(struct input_event));
	if(ret < 0){
		printf("%s:%d\n", __func__, __LINE__);
		return ret;//error process.
	}
	
	gettimeofday(&key_event.time, NULL);
	key_event.type = EV_SYN;
	key_event.code = SYN_REPORT;
	key_event.value = 0;//event status sync
	ret = write(uinput_fd, &key_event, sizeof(struct input_event));
	if(ret < 0){
		printf("%s:%d\n", __func__, __LINE__);
		return ret;//error process.
	}
	
	return 0;
}
