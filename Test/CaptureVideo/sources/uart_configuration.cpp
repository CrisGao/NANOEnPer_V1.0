#include "uart_configuration.h"
#include <stdio.h>
#include <ctime>

#define filename(x) strrchr(x,'/')

#define __DEBUG__ 1
#if __DEBUG__ 
#define DEBUG(format,...) printf("[File:" __FILE__ ",LINE:%03d]" format "\n", __LINE__, ##__VA_ARGS__)
#define LOGG(s) printf("[%s,%d] %s\n",filename(__FILE__),__LINE__,s)
#else
#define DEBUG(format,...)
#define LOGG(s) NULL
#endif

//int n=0;

double VehicleSpeed = 0.00;

int8_t jetsonSerial::transceiver_send =0;

int8_t jetsonSerial::transceiver_receiver = 0;

int jetsonSerial::fd_send =0;

int jetsonSerial::fd_receiver =0;

jetsonSerial* jetsonSerial::ptr_Instance = nullptr;

jetsonSerial::jetsonSerial()
{
       srand(time(0));
};

jetsonSerial::~jetsonSerial()
{

       UART0_Close(fd_send);

	UART0_Close(fd_receiver);
}


bool jetsonSerial::Transceriver_UART_init(char *port, int speed,int flow_ctrl,int databits,int stopbits,int parity,int flag)
{
	int fd;
	int8_t transceiver;
       transceiver = IM_CreateTransceiverDynamic(50, 0, 50, 0); //创建接收器

       if (!ResetReceiverCipherTable(transceiver, CIPHER_TABLE))
       {
              std::cout << "had success load tabel" << std::endl;
       }

       fd = UART0_Open(fd,port);

       int err;

        do
       {
              err = UART0_Init( fd, 9600, 0, 8, 1, 'N'); 
              printf("Set port exactly!\n");
		n++;
		if(n==10)
		break;
       }while(FALSE == err || FALSE == fd);

       if(n==10)
		return false;

       
	if(flag == RECEIVER)
	{
		fd_receiver = fd;
		
		transceiver_receiver = transceiver;
			
	}
	else if(flag == SEND)
	{
		fd_send = fd;

		transceiver_send = transceiver;
	}

		return true;

	

}


void jetsonSerial::Send_TriggerVoice(int flag,int voice)
{
	uint8_t send_cmd = 0x11;

	uint8_t send_params[2]; 
	
	if(flag ==1)
	{
		send_params[0] = 0x01; //open the voice 200
		send_params[1] = (uint8_t)voice;
		
	}

	else if(flag == 0)
	{
		send_params[0] = 0x00; //close the voice
		
	}
	
	IM_PackDataToTransmitter(transceiver_send,rand()%256,0XFFFFFFFF,3,&send_cmd,1,send_params,sizeof(send_params),PARITY);

	uint8_t *send_data;

	uint16_t send_length;

	if(IM_GetPacketFromTransmitter(transceiver_send,&send_data,&send_length,0)>0)
	{
		UART0_Send(fd_send,(char*)send_data,send_length);

		IM_FreePacketFromTransmitter(transceiver_send);

	}

}
/*****************************************************************

* 名称：                    UART0_Open

* 功能：                    打开串口并返回串口设备文件描述

* 入口参数：            fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)

* 出口参数：            正确返回为1，错误返回为0

*****************************************************************/
int jetsonSerial::UART0_Open(int fd,char* port)
{
   
         fd = open( port, O_RDWR|O_NOCTTY|O_NONBLOCK|O_SYNC|O_NDELAY);
         if (FALSE == fd)
                {
                       perror("Can't Open Serial Port");
                       return(FALSE);
                }
     //恢复串口为阻塞状态                               
     if(fcntl(fd, F_SETFL, 0) < 0)
                {
                       printf("fcntl failed!\n");
                     return(FALSE);
                }     
         else
                {
                  printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));
                }
      //测试是否为终端设备    
      if(0 == isatty(STDIN_FILENO))
                {
                       printf("standard input is not a terminal device\n");
                 // return(FALSE);
                }
  else
                {
                     printf("isatty success!\n");
                }              
  printf("fd->open=%d\n",fd);
  return fd;
}

void jetsonSerial::UART0_Close(int fd)
{
    close(fd);
}

int jetsonSerial::UART0_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{
   
       int   i;
       int   status;
       int   speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};
       int   name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};
         
       struct termios options;
   
    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.
    */
    if  ( tcgetattr( fd,&options)  !=  0)
       {
          perror("SetupSerial 1");    
          return(FALSE); 
       }
  
    //设置串口输入波特率和输出波特率
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
                {
                     if  (speed == name_arr[i])
                            {             
                                 cfsetispeed(&options, speed_arr[i]); 
                                 cfsetospeed(&options, speed_arr[i]);  
                            }
              }     
   
    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;

  
    //设置数据流控制
    switch(flow_ctrl)
    {
      
       case 0 ://不使用流控制
              options.c_cflag &= ~CRTSCTS;
              break;   
      
       case 1 ://使用硬件流控制
              options.c_cflag |= CRTSCTS;
              break;
       case 2 ://使用软件流控制
              options.c_cflag |= IXON | IXOFF | IXANY;
              break;
    }
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {  
       case 5    :
                     options.c_cflag |= CS5;
                     break;
       case 6    :
                     options.c_cflag |= CS6;
                     break;
       case 7    :    
                 options.c_cflag |= CS7;
                 break;
       case 8:    
                 options.c_cflag |= CS8;
                 break;  
       default:   
                 fprintf(stderr,"Unsupported data size\n");
                 return (FALSE); 
    }
    //设置校验位
    switch (parity)
    {  
       case 'n':
       case 'N': //无奇偶校验位。
                 options.c_cflag &= ~PARENB; 
                 options.c_iflag &= ~INPCK;    
                 break; 
       case 'o':  
       case 'O'://设置为奇校验    
                 options.c_cflag |= (PARODD | PARENB); 
                 options.c_iflag |= INPCK;             
                 break; 
       case 'e': 
       case 'E'://设置为偶校验  
                 options.c_cflag |= PARENB;       
                 options.c_cflag &= ~PARODD;       
                 options.c_iflag |= INPCK;      
                 break;
       case 's':
       case 'S': //设置为空格 
                 options.c_cflag &= ~PARENB;
                 options.c_cflag &= ~CSTOPB;
                 break; 
        default:  
                 fprintf(stderr,"Unsupported parity\n");    
                 return (FALSE); 
    } 
    // 设置停止位 
    switch (stopbits)
    {  
       case 1:   
                 options.c_cflag &= ~CSTOPB; break; 
       case 2:   
                 options.c_cflag |= CSTOPB; break;
       default:   
                       fprintf(stderr,"Unsupported stop bits\n"); 
                       return (FALSE);
    }
   
     //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;
  
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//我加的
    //options.c_lflag &= ~(ISIG | ICANON);
   
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */  
    options.c_cc[VMIN] = 8; /* 读取字符的最少个数为1 */
   
    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
    tcflush(fd,TCIOFLUSH);
   
    //激活配置 (将修改后的termios数据设置到串口中）
    if (tcsetattr(fd,TCSANOW,&options) != 0)  
           {
               perror("com set error!\n");  
              return (FALSE); 
           }
    return (TRUE); 
}

/*************************************************
* 名称：                  UART0_Recv

* 功能：                接收串口数据

* 入口参数：        fd                  :文件描述符    

*                              rcv_buf     :接收串口中数据存入rcv_buf缓冲区中

*                              data_len    :一帧数据的长度

* 出口参数：        正确返回为1，错误返回为0
****************************************************/
int jetsonSerial::UART0_Recv(int fd,uint8_t *rcv_buf,int data_len)
{
    int len,fs_sel;
    fd_set fs_read;
   
    struct timeval time;
   
    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);
   
    time.tv_sec = 10;
    time.tv_usec = 0;
   
    //使用select实现串口的多路通信
    fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
    if(fs_sel)
       {
              len = read(fd,rcv_buf,data_len);
	     // printf("I am right!(version1.2) len = %d fs_sel = %d\n",len,fs_sel);

              return len;
       }
    else
       {
	      printf("Sorry,Cannot receive datas!");
              return FALSE;
       }     
}


int jetsonSerial::UART0_Send(int fd,char *send_buf,int data_len)
{
    int len = 0;
   
    len = write(fd,send_buf,data_len);
    if (len == data_len )
              {
                     return len;
              }     
    else   
        {
               
                tcflush(fd,TCOFLUSH);
                return FALSE;
        }
   
}
/*******************************************************************

* 名称：                  UART0_Init()

* 功能：                串口初始化

* 入口参数：        fd           文件描述符   

*               speed     串口速度

*                              flow_ctrl   数据流控制

*               databits    数据位   取值为 7 或者8

*                           stopbits   停止位   取值为 1 或者2

*                           parity     效验类型 取值为N,E,O,,S                    

* 出口参数：        正确返回为1，错误返回为0

*******************************************************************/
int jetsonSerial::UART0_Init(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{
   // int err;
    //设置串口数据帧格式
    if (UART0_Set(fd,9600,0,8,1,'N') == FALSE)
       {                                                         
        return FALSE;
       }
    else
       {
               return  TRUE;
        }
}
