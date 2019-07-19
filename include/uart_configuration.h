#ifndef _UART_CONFIGURATION_H_
#define _UART_CONFIGURATION_H_
//串口相关的头文件
#include<stdio.h>      /*标准输入输出定义*/
#include<stdlib.h>     /*标准函数库定义*/
#include<unistd.h>     /*Unix 标准函数定义*/
#include<sys/types.h> 
#include<sys/stat.h>   
#include<fcntl.h>      /*文件控制定义*/
#include<termios.h>    /*PPSIX 终端控制定义*/
#include<errno.h>      /*错误号定义*/
#include<string.h>
#include <iostream> 


#include <stdint.h>
#include "im_protocol.h"
#include "im_packet_send_receive_method.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>

#define FALSE -1
#define TRUE 0

typedef unsigned char uint8_t;
//typedef int(*CallBackFun)();//为回调函数命名，类型命名CallBackFun,参数为Char*p
 typedef struct IMSS_REALTIME_INFO
{
       uint16_t voltage; //当前电池压 单位 0.01V
       int16_t current; //当前电池输出总流 单位 0.01A
       int16_t speed; //当前车速 单位 0.01km/h
       uint16_t mileage; //本次里程 单位 0.01km
       uint16_t remainder_mileage; //剩余里程 单位 0.01km
       uint8_t battery_percent; //剩余电量 单位 1%
       int8_t mos_temperature; //控制器 (MOS管)温度 单位 °C
       int8_t motor_temperature; //电机温度 单位 °C
       int8_t battery_temperature; //电池温度 单位 °C
       uint8_t light_intensity; //光照强度 单位 1%
       uint8_t tail_light_battery_percent; //尾灯电池剩余量 单位 1%
       //实时状 态
       uint8_t state_run_mode : 2; //0 失能（油门 无效 刹车 有效 ，显示屏熄灭）
       //1 使能（ 油门 有效 刹车有，显示屏工作 ，车辆可以正常使用）
       //2 锁车 （油门无效刹车有，显示屏熄灭轮抱死停后执行 （油门无效刹车有，显示屏熄灭轮抱死停后执行 ）
       //3 休眠（ 油门无效 刹车无效，显示屏熄灭辆不自锁电源 刹车无效，显示屏熄灭辆不自锁电源 ）
       uint8_t state_motor_work : 1; //0 电机在非工作状态 1 电机在工作状态 (加速，刹车 锁加速，刹车 锁)
       uint8_t state_brake : 1; //0 非刹车状态 1 正在刹车
       uint8_t state_charge;
       uint8_t state_lamp : 1; //0 照明灯开启状态 1 照明灯关闭状态
       uint8_t state_tail_light : 2; //0 尾灯关闭状态 1 尾灯常亮状态
       //2 尾灯闪烁状态 3 尾灯呼吸状态
       uint8_t state_auto_cruise : 1; //0 非自动巡航状态 1 自动巡航状态
       uint8_t state_reserve : 7; //保留状态
       //故障信息
       uint16_t error_controller : 1; //0 控制器正常 1 控制器故障
       uint16_t error_motor : 1; //0 电机正常 1 电机故障
       uint16_t error_battery : 1; //0 电池正常 1 电池故障
       uint16_t error_voltage : 2; //0 电压正常 1 欠压故障 2 过压故障 3 保留
       uint16_t error_brake_handle : 1; //0 刹车把手正常 1 刹车把手故障
       uint16_t error_throttle_handle : 1; //0 油门把手正常 1 油门把手故障
       uint16_t error_communication1 : 1; //0 IOT模块数据接收正常 模块数据接收正常 1 无法收到 IOT模块指令
       uint16_t error_communication2 : 1; //0 仪表 -控制器通讯正常 1 仪表 -控制器通讯故障
       //如无 仪表则此项故障如无
       uint16_t error_mos_over_temp : 1; //0 MOS温度正常 1 MOS温度过高
       uint16_t error_motor_over_temp : 1; //0 电机温度正常 1 电机温度过高
       uint16_t error_battery_over_temp: 1; //0 电池温度正常 1 电池温度过高
       uint16_t error_tail_light_lost : 1; //0 尾灯正常 1 尾灯通讯故障
       uint16_t error_reserve;
}CAR_INFO;

void*  CallGet_Speed(void *ptr);

class jetsonSerial
{
   public:
       static jetsonSerial* getInstance()
       {
		if(ptr_Instance == NULL)
		{
			ptr_Instance = new jetsonSerial();
			return ptr_Instance;
		}
       }

       jetsonSerial();

       ~jetsonSerial();

       void Transceriver_UART_init(char *port, int speed,int flow_ctrl,int databits,int stopbits,int parity);

       int UART0_Open(int fd,char* port);

       void UART0_Close(int fd);

       int UART0_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);

       int UART0_Recv(int fd,uint8_t *rcv_buf,int data_len);

       int UART0_Send(int fd,char *send_buf,int data_len);

       int UART0_Init(int fd, int speed,int flow_ctrl,int databits,int stopbits,int parity);

       pthread_t startThread();

       void Send_TriggerVoice();  //use uart to trigger the vehicle voice and lighting

       uint8_t receiv_buf[100];

       CAR_INFO GetInfo;

       static int8_t transceiver;

       static int fd;//文件描述符
     

   private:
	static 	jetsonSerial* ptr_Instance;
	
        pthread_t uart_thread_id;
        
        static const int SPEED = 0; 


     uint8_t CIPHER_TABLE[256] =
{
	0xED,0xB9,0xF2,0x58,0x8B,0xE3,0xC5,0xEC,0x14,0xC0,0x9D,0x42,0x60,0x17,0x0F,0x9E,
	0x0B,0x98,0xDC,0x16,0x9D,0xDF,0x66,0xD1,0xC3,0x9C,0xC7,0xBF,0x4D,0xFD,0xBD,0xCE,
	0xEB,0x95,0xE6,0x96,0x25,0x4F,0xA8,0x8D,0x52,0x49,0x96,0xD0,0xC2,0x80,0xBB,0xCC,
	0x90,0x02,0x09,0x3E,0x1E,0x7B,0x39,0xF3,0x47,0x4A,0x20,0x00,0x6F,0x9B,0x2F,0xA9,
	0x1A,0x50,0xA2,0x0E,0x06,0x21,0x9C,0xFB,0x1E,0x59,0x76,0x4A,0x83,0xD3,0xCA,0x4D,
	0xDC,0xFB,0x31,0xC2,0xA6,0xDE,0x14,0x11,0x60,0x99,0x3E,0xC6,0x7A,0x41,0xC5,0x5B,
	0xF6,0xCB,0xD9,0x4D,0x4F,0x62,0x2B,0x15,0x3D,0xAF,0x4D,0xCB,0x7A,0x6D,0x32,0x40,
	0xAD,0x38,0xE7,0x7A,0x3B,0x1D,0x25,0x19,0x3D,0x3E,0xF4,0xE1,0xD5,0x32,0x8E,0xF2,
	0xF1,0x40,0xA4,0x4E,0xD9,0x83,0xDC,0x90,0x44,0x64,0x6C,0xEB,0x02,0xCC,0xA6,0x6E,
	0x6E,0x2B,0x6F,0x8B,0x67,0x09,0x3B,0xD2,0x4E,0x8C,0x3F,0x95,0x99,0x9C,0xC5,0x66,
	0x67,0x7B,0xA5,0xD7,0xBA,0x89,0xE9,0x72,0x4D,0xA1,0xD6,0x57,0xF6,0x35,0x54,0x9D,
	0x2D,0xE0,0x5B,0x6A,0x68,0x66,0x05,0xA8,0x0F,0x07,0xEB,0xEE,0x56,0x31,0x77,0x94,
	0x05,0xF8,0x07,0xED,0xE7,0x87,0x98,0x3F,0x3E,0xB2,0xB1,0x31,0xFA,0x2D,0x94,0x7D,
	0x99,0xC6,0x1F,0x83,0x66,0xD9,0x11,0x32,0x15,0x8B,0xF8,0x04,0x61,0x8E,0x04,0x81,
	0xF0,0x92,0x80,0xD9,0x7E,0xB7,0xD4,0x24,0xD1,0xA4,0xAD,0x21,0x89,0x81,0x55,0x8D,
	0x50,0x56,0x6A,0x28,0xD8,0xD5,0x82,0x89,0x5A,0x1C,0xEA,0x42,0x2A,0x08,0x6A,0xDF,
}; 

};


#endif  //_UART_CONFIGURATION_H_
