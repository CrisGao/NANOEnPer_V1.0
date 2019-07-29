#include "uart_configuration.h"
#include "VideoImg.h"

#include <iostream>
#include <string.h>
#include <unistd.h> /*Unix 标准函数定义*/

#include <signal.h>

#define filename(x) strrchr(x,'/')

#define __DEBUG__ 1
#if __DEBUG__ 
#define DEBUG(format,...) printf("[File:" __FILE__ ",LINE:%03d]" format "\n", __LINE__, ##__VA_ARGS__)
#define LOGG(s) printf("[%s,%d] %s\n",filename(__FILE__),__LINE__,s)
#else
#define DEBUG(format,...)
#define LOGG(s) NULL
#endif

extern double VehicleSpeed;

VideoImg* JetsonVideo; 
jetsonSerial* JetsonUart;

void server_on_exit(void);

void signal_exit_handler(int sig);

void signal_crash_handler(int sig);


pthread_t uart_id, videoImg_id, Classify_id;

bool ifExit = false;

int main()
{

	//pthread_t uart_id, videoImg_id, Classify_id;

	#if 1
	std::cout << "---------- Init UART ----------" << std::endl;
	JetsonUart = jetsonSerial::getInstance();

	char *OpenFile = "/dev/ttyTHS1";
	if(!JetsonUart->Transceriver_UART_init(OpenFile, 9600, 0, 8, 1, 'N'))  //configurate the UART
	return -1;

	std::cout << "---------- Start Get data through UART ----------" << std::endl;
	uart_id = JetsonUart->startThread();
	#endif
	
	JetsonUart->Send_TriggerVoice(1);//Running the program will voice
	sleep(1);
	JetsonUart->Send_TriggerVoice(0);
	
	std::cout << "---------- InitCamera ----------" << std::endl;
	JetsonVideo = new VideoImg();
	if (!JetsonVideo->InitCamera(3280, 2464, 1280, 720, 20, 2))  // configurate the CSI camera stream
	{
		LOGG("CANNOT INIT CAMERA");
		return -1;
	}



	atexit(server_on_exit);
	signal(SIGINT,signal_exit_handler);  //Response the "kill the process"
	signal(SIGTERM,signal_exit_handler); //Response the "Ctrl+C end foreground process"
	
/*
	//non normal exit
	signal(SIGABRT,signal_crash_handler);//use aboart to exit
	signal(SIGSEGV,signal_crash_handler);//non illegal access memory
	signal(SIGBUS,signal_crash_handler);//bus error

*/
#if 0
std::cout << "---------- Start Prediction ----------" << std::endl;
		Classify_id = JetsonVideo->startThread_classify();
#endif
	std::cout << "---------- START for Work ----------" << std::endl;
	while (1)
	{
		#if 1
		if (!JetsonVideo->startCamera())
		{
			break;
		}
		#endif
		if(JetsonVideo->ifGetImageFromCamera())
		{
			std::cout << "---------- Start Prediction ----------" << std::endl;
			Classify_id = JetsonVideo->startThread_classify();
		}
   		if(cv::waitKey(30)>0)
		break;
	}
	pthread_cancel(uart_id);
	pthread_join(uart_id, NULL);
	pthread_exit(NULL);
	return 0;
}

void server_on_exit(void)
{

	pthread_cancel(videoImg_id);
	pthread_cancel(Classify_id);
	pthread_cancel(uart_id);

	JetsonVideo->deleteSources();
        delete JetsonVideo;
	delete JetsonUart;
	JetsonVideo = NULL;
	JetsonUart  = NULL;
	LOGG("HAD SUCCESS EXIT");	
}

void signal_exit_handler(int sig)
{
	
	exit(0);
}

void signal_crash_handler(int sig)
{
	JetsonVideo->deleteSources ();
        delete JetsonVideo;
	delete JetsonUart;
	JetsonVideo = NULL;
	JetsonUart  = NULL;
	LOGG("UNNORMA EXIT");
	exit(-1);
}

