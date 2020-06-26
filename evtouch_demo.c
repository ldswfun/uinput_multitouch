#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <linux/uinput.h>
#include <linux/input.h>
 
 
#define KEY_CUSTOM_UP 0x20
#define KEY_CUSTOM_DOWN 0x30

#define TS_MAX_WIDTH 720
#define TS_MAX_HEIGHT 1280

static int debug = 0;

typedef enum ts_action_type {
	TYPE_DOWN,
	TYPE_MOVE,
	TYPE_UP,
}ts_action_type;


typedef struct  vdagentd_touchdata{
	uint8_t id;      // touch point id
    uint8_t pressure; 
	uint8_t type;   //  down(0)/move(1)//up(2)
    uint32_t x;
    uint32_t y;
	uint32_t touch_major;
    uint32_t touch_minor;
} vdagentd_touchdata;

int open_uinput_dev(void)
{
	
	int fd;

	fd = open("/dev/uinput", O_RDWR | O_NDELAY);
	if(fd < 0)
		return -1;//error process.

	return fd;

}

int creat_user_key_uinput(int uinput_fd)
{
	int i;
	int ret = 0;
		
	struct uinput_user_dev uinput_dev;
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


int create_touch_uinput(int uinput_fd)
{
	int ret;
	struct uinput_user_dev uinput_dev;
	// configure touch device event properties
	memset(&uinput_dev, 0, sizeof(uinput_dev));
	//设备的别名
	strncpy(uinput_dev.name, "UinputTouchScreen", UINPUT_MAX_NAME_SIZE);
	
	uinput_dev.id.version = 4;
	uinput_dev.id.bustype = BUS_VIRTUAL;


	// getevent -v32  /dev/eventX
	uinput_dev.absmin[ABS_X] = 0;
	uinput_dev.absmax[ABS_X] = 32767; 
	uinput_dev.absmin[ABS_Y] = 0;
	uinput_dev.absmax[ABS_Y] = 32767;
	uinput_dev.absmin[ABS_Z] = 0;
	uinput_dev.absmax[ABS_Z] = 1;
	
	uinput_dev.absmin[ABS_MT_SLOT] = 0;
	uinput_dev.absmax[ABS_MT_SLOT] = 9; // MT代表multi touch 多指触摸 最大手指的数量我们设置9
	uinput_dev.absmin[ABS_MT_TOUCH_MAJOR] = 0;
	uinput_dev.absmax[ABS_MT_TOUCH_MAJOR] = 15;
	uinput_dev.absmin[ABS_MT_POSITION_X] = 0; // 屏幕最小的X尺寸
	uinput_dev.absmax[ABS_MT_POSITION_X] = 32767; //TS_MAX_WIDTH; // 屏幕最大的X尺寸
	uinput_dev.absmin[ABS_MT_POSITION_Y] = 0; // 屏幕最小的Y尺寸
	uinput_dev.absmax[ABS_MT_POSITION_Y] = 32767; //TS_MAX_HEIGHT; //屏幕最大的Y尺寸
	uinput_dev.absmin[ABS_MT_TRACKING_ID] = 0;
	uinput_dev.absmax[ABS_MT_TRACKING_ID] = 65535;//按键码ID累计叠加最大值
	uinput_dev.absmin[ABS_MT_PRESSURE] = 0;   
	uinput_dev.absmax[ABS_MT_PRESSURE] = 256;     //屏幕按下的压力值

	// Setup the uinput device
	ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);   //该设备支持按键
	ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);   //支持鼠标
	ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN); //支持同步，用于report
	// for key
	ioctl(uinput_fd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
	ioctl(uinput_fd, UI_SET_KEYBIT, KEY_VOLUMEUP);
	ioctl(uinput_fd, UI_SET_KEYBIT, KEY_POWER);
	ioctl(uinput_fd, UI_SET_KEYBIT, KEY_BACK);

	// Touch
	ioctl (uinput_fd, UI_SET_EVBIT,  EV_ABS); //支持触摸
	ioctl (uinput_fd, UI_SET_ABSBIT,  ABS_X); 
	ioctl (uinput_fd, UI_SET_ABSBIT,  ABS_Y); 
	ioctl (uinput_fd, UI_SET_ABSBIT,  ABS_Z); 
	//ioctl (uinput_fd, UI_SET_ABSBIT,  ABS_PRESSURE);
	
	ioctl (uinput_fd, UI_SET_ABSBIT, ABS_MT_SLOT);
	ioctl (uinput_fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
	ioctl (uinput_fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
	ioctl (uinput_fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
	ioctl (uinput_fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
	ioctl (uinput_fd, UI_SET_ABSBIT, ABS_MT_PRESSURE);    
	ioctl (uinput_fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

	/* Create input device into input sub-system */
	ret = write(uinput_fd, &uinput_dev, sizeof(struct uinput_user_dev));
	if(ret < 0)
	{
		printf("%s:%d\n", __func__, __LINE__);
		return ret;//error process.
	}
	
	ret = ioctl(uinput_fd, UI_DEV_CREATE);
	if(ret < 0)
	{
		printf("%s:%d\n", __func__, __LINE__);
		close(uinput_fd);
		return ret;//error process.
	}

	return 0;

}


int report_key(int uinput_fd , unsigned int type, unsigned int keycode, unsigned int value)
{
	struct input_event key_event;
	int ret;
	
	memset(&key_event, 0, sizeof(struct input_event));
	
	gettimeofday(&key_event.time, NULL);
	key_event.type = type;
	key_event.code = keycode;
	key_event.value = value;
	ret = write(uinput_fd, &key_event, sizeof(struct input_event));
	if(ret < 0)
	{
		printf("%s:%d\n", __func__, __LINE__);
		return ret;//error process.
	}
	
	gettimeofday(&key_event.time, NULL);
	key_event.type = EV_SYN;
	key_event.code = SYN_REPORT;
	key_event.value = 0;//event status sync
	ret = write(uinput_fd, &key_event, sizeof(struct input_event));
	if(ret < 0)
	{
		printf("%s:%d\n", __func__, __LINE__);
		return ret;//error process.
	}
	
	return 0;
}

int uinput_write_event(int fd, int type, int code, int value)
{
    struct input_event ev; //input_event定义在 /usr/include/linux/input.h
    memset(&ev, 0, sizeof(struct input_event));
	//将事件存入结构体
	gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    //写入到设备的事件文件/dev/uinput，在/dev/input/eventX文件中可查看
    if (write(fd, &ev, sizeof(struct input_event)) < 0) 
	{
        char * mesg = strerror(errno);
        printf("uinput errormag info :%s\n",mesg);
        return -1;
    }
    
	return 1;
}


void key_input_test_case(int uinput_fd)
{
	char kbd_buf[32];
	printf("input up/down:");

	while(1)
	{
		fgets(kbd_buf, 32, stdin);
		if(strncmp(kbd_buf, "up", 2) == 0)
		{

			report_key(uinput_fd, EV_KEY, KEY_VOLUMEUP, 1);// Report BUTTON A CLICK - PRESS event
			report_key(uinput_fd, EV_KEY, KEY_VOLUMEUP, 0);// Report BUTTON A CLICK - RELEASE event
		}else if(strncmp(kbd_buf, "down", 3) == 0)
		{
			report_key(uinput_fd, EV_KEY, KEY_VOLUMEDOWN, 1);
			report_key(uinput_fd, EV_KEY, KEY_VOLUMEDOWN, 0);
		}else{
			printf("unsupport key\n");
		}
	}	
}


void do_touch_zoom_inout(int uinput_fd, int type)
{
	int id = 0;
	
    int x0 = 600, y0 = 600, x1 = 1200, y1 = 600; // original position

    // to wait or not
    usleep(20*1000);
    id += 1;
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, 0);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, id);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 4);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 80);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, x0 );
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, y0 );
    uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);// sync
    usleep(1*1000);
    id += 1;
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, 1);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, id);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 5);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 80);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, x1 );
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, y1 );
    uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);

    //two finger move left and right 
    usleep(20*1000);
    for (int i=0; i<50; i++)
	{
        if (type == 0)
		{ // zoom in 
            x0 -= 3;
            x1 += 3;
        }else { // zoom out
            x0 += 3;
            x1 -= 3;
        }

		// update position of two points
        usleep(40*1000);
        uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, 0);
        uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 80);
        uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, x0 );
        uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);

        uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, 1);
        uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 80);
        uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, x1 );
        uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
    }

    //two finger leave from screen
    usleep(10*1000);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, 1);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 0);// no preessure
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, x1 );
    uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, -1);//-1 mean finfer up
    uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
    usleep(10*1000);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, 0);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 0);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, x0 );
    uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
    uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
}

void touch_zoom_test_case(int uinput_fd)
{
	char kbd_buf[32];
	printf("input in/out:");

	while(1)
	{
		fgets(kbd_buf, 32, stdin);
		if(strncmp(kbd_buf, "in", 2) == 0)
		{
			do_touch_zoom_inout(uinput_fd, 0); // zoom in --bigger
			
		}else if(strncmp(kbd_buf, "out", 3) == 0)
		{ 
			do_touch_zoom_inout(uinput_fd, 1);  //zoom out
			
		}else{
			printf("unsupport operation\n");
		}
	}
}




static int trackid = 1;
void do_client_touch(int uinput_fd, vdagentd_touchdata *ts, int ts_num)
{ 
	int i; 
	
	
	if(uinput_fd < 0 || !ts)
		return ;

	if(access("ts_debug.ini",0)==0)
		debug = 1;

	if(debug)
		printf("ts_num : %d, ts : type=%d, id=%d,  x = %d, y=%d, pressure = %d, t_major = %d, t_minor = %d", 
					ts_num, ts->type, ts->id, ts->x, ts->y, ts->pressure, ts->touch_major, ts->touch_minor);

	//  1, btn_touch(down),  2, multi slot, trackid, major, minor, x, y,  3,  btn_touch(down)
	for(i=0; i<ts_num; i++)
	{
		
		switch(ts[i].type)
		{
			case TYPE_DOWN:  
				//multi touch need slot， no need in single touch mode
				if(ts_num > 1)
					uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, ts->id);

				uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, trackid++);
				if(i < 1)//  first down event sequence
				{
					if(ts_num > 1)
					{
						uinput_write_event(uinput_fd, EV_KEY, BTN_TOUCH,  1);// fist event
						uinput_write_event(uinput_fd, EV_KEY, BTN_TOOL_FINGER,	1);
					}

					uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TOUCH_MAJOR, ts->touch_major+0xaf); // fake
					uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 0x81);
					
					//uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TOUCH_MINOR, ts->touch_minor+2 ); // fake	
				}
				
				uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, ts->x);
				uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, ts->y);

				// sync
				uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
				break;
			case TYPE_MOVE:
				if(ts_num > 1)
					uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, ts->id);
				
				uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_X, ts->x);
				uinput_write_event(uinput_fd, EV_ABS, ABS_MT_POSITION_Y, ts->y);
				uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
				break;
				
			case TYPE_UP:
				if(ts_num > 1)// muti touch
					uinput_write_event(uinput_fd, EV_ABS, ABS_MT_SLOT, ts->id);

				uinput_write_event(uinput_fd, EV_ABS, ABS_MT_PRESSURE, 0);
				uinput_write_event(uinput_fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
				
				if(i == (ts_num-1))// last event
				{	
					if(ts_num > 1)
					{
						uinput_write_event(uinput_fd, EV_KEY, BTN_TOUCH,  0);
						uinput_write_event(uinput_fd, EV_KEY, BTN_TOOL_FINGER,	0);
					}
					// sync
					uinput_write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
				}
				break;
		
		}
	}
	
}

void client_json_touch_test_case(int uinput_fd)
{
	char kbd_buf[32];
	int cout = 0;
	
	// prepare data
	vdagentd_touchdata single_points_down_data = {
			.id = 0, 
		    .pressure  = 10, 
			.type = TYPE_DOWN,   
		    .x = 0x413e,   // home x, y
		    .y = 0x7cff,
			.touch_major = 3,
		    .touch_minor = 3,
	};
	vdagentd_touchdata single_points_up_data = {
			.id = 0, 
		    .pressure  = 10, 
			.type = TYPE_UP,  
		    .x = 0x413e,
		    .y = 0x7cff,
			.touch_major = 3,
		    .touch_minor = 3,

	};
	vdagentd_touchdata single_points_move_data[10] = {
		[0] = {
			.id = 0, 
		    .pressure  = 10, 
			.type = TYPE_UP,  
		    .x = 0x272,
		    .y = 0x642,
			.touch_major = 3,
		    .touch_minor = 3,
		},
	};
	
	vdagentd_touchdata three_points_updown_data[3] = {


	};

	vdagentd_touchdata single_points_line_data[3] = {
	
	
	};
	vdagentd_touchdata three_points_line_data[3] = {

	};

	
	
	printf("input 1(single)/2(mutil) down/up:");

	while(1)
	{
		fgets(kbd_buf, 32, stdin);
		if(strncmp(kbd_buf, "1", 1) == 0)
		{	
			if((cout++)%2 == 0)
			{	
				// show all app
				single_points_down_data.x = 0x3d82;  
				single_points_down_data.y = 0x6de5;
				do_client_touch(uinput_fd, &single_points_down_data,  1);
				do_client_touch(uinput_fd, &single_points_up_data,  1);
			}else{
				single_points_down_data.x = 0x3f76;
				single_points_down_data.y = 0x7ae5;
				do_client_touch(uinput_fd, &single_points_down_data,  1);
				do_client_touch(uinput_fd, &single_points_up_data,  1);
			}
			
		}else if(strncmp(kbd_buf, "2", 1) == 0)
		{ 
			do_touch_zoom_inout(uinput_fd, 1);  //缩小
			
		}else{
			printf("unsupport operation\n");
		}
	}
}



int main(int argc, char *argv[])
{
	int ret = 0;
	int uinput_fd;

	

	uinput_fd = open_uinput_dev();
	if(uinput_fd < 0)
	{
		printf("open error : %s\n", strerror(errno));
		exit(1);	
	}
	
	ret = create_touch_uinput(uinput_fd);
	if(ret < 0)
	{
		printf("%s:%d\n", __func__, __LINE__);
		return -1;//error process.
	}

	client_json_touch_test_case(uinput_fd);
	//touch_zoom_test_case(uinput_fd);

	close(uinput_fd);
	return 0;
}


