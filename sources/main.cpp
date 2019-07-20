#include "uart_configuration.h"
#include "VideoImg.h"

#include <iostream>
#include <string.h>
#include <unistd.h> /*Unix 标准函数定义*/

#include <signal.h>

extern double VehicleSpeed;

VideoImg *JetsonVideo; 
jetsonSerial *JetsonUart;

void server_on_exit(void);

void signal_exit_handler(int sig);

void signal_crash_handler(int sig);

int main()
{

	pthread_t uart_id, videoImg_id, Classify_id;

	
	#if 0
	std::cout << "---------- Init UART ----------" << std::endl;
	JetsonUart = jetsonSerial::getInstance();

	char *OpenFile = "/dev/ttyTHS1";
	JetsonUart->Transceriver_UART_init(OpenFile, 9600, 0, 8, 1, 'N');  //configurate the UART

	std::cout << "---------- Start Get data through UART ----------" << std::endl;
	uart_id = JetsonUart->startThread();
	pthread_join(uart_id, NULL);
	#endif

	std::cout << "---------- Loading model sysfile ----------" << std::endl;
	string model_file = "../data/deploy.prototxt";
	string trained_file = "../data/3caffe_train_iter_600000.caffemodel";
	string mean_file = "../data/mean.binaryproto";
	string label_file = "../data/label.txt";
	JetsonVideo->Init_Classify(model_file, trained_file, mean_file, label_file); //load the caffe model

	
	std::cout << "---------- InitCamera ----------" << std::endl;
	JetsonVideo = new VideoImg();
	if (!JetsonVideo->InitCamera(3280, 2464, 1280, 720, 20, 2))  // configurate the CSI camera stream
	{
		return -1;
	}
	JetsonVideo->Init_VideoWriteFileStorage(1280,720);  //configurate the video resolution;

	atexit(server_on_exit);
	signal(SIGINT,signal_exit_handler);  //Response the "kill the process"
	signal(SIGTERM,signal_exit_handler); //Response the "Ctrl+C end foreground process"
	
	//ignore SIGPIPE
	signal(SIGPIPE,SIG_IGN);
	
	//non normal exit
	signal(SIGABRT,signal_crash_handler);//use aboart to exit
	signal(SIGSEGV,signal_crash_handler);//non illegal access memory
	signal(SIGBUS,signal_crash_handler);//bus error
	
	bool ifStart = false;
	bool ifCancel = false;

	while (true)
	{
		if (!JetsonVideo->startCamera())
		{
			break;
		}
		
		if(VehicleSpeed>0)
				ifStart = true;
		else
				ifCancel = true;
		
		if(ifStart)
		{
			std::cout << "---------- Prediction ----------" << std::endl;
			Classify_id = JetsonVideo->startThread_classify();

			std::cout << "---------- SaveVideoSpeed ----------" << std::endl;
			videoImg_id = JetsonVideo->startThread_saveVideoSpeed();
			
			ifStart = false;
		}
		
		if(ifCancel)
		{

			pthread_cancel(Classify_id);
			pthread_cancel(videoImg_id);
			pthread_join(Classify_id,NULL);
			pthread_join(videoImg_id,NULL);
			ifCancel = false;

		}

	}
	
	return 0;
}

void server_on_exit(void)
{
	JetsonVideo->deleteSources();
        delete JetsonVideo;
	delete JetsonUart;
	JetsonVideo = NULL;
	JetsonUart  = NULL;	
}

void signal_exit_handler(int sig)
{
	//JetsonVideo->deleteSources();
	exit(0);
}

void signal_crash_handler(int sig)
{
	exit(-1);
}
