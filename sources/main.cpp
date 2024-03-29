#include "uart_configuration.h"
#include "VideoImg.h"

#include <iostream>
#include <string.h>
#include <unistd.h> /*Unix 标准函数定义*/
#include <signal.h>




extern double VehicleSpeed;

VideoImg* JetsonVideo; 
jetsonSerial* JetsonUart;

void server_on_exit(void);

void signal_exit_handler(int sig);

pthread_t uart_id, videoImg_id, Classify_id;

bool ifExit = false;

int main(int argc,char* argv[])
{
	google::InitGoogleLogging(argv[0]);
	google::SetLogDestination(google::GLOG_INFO,"/home/leon/NANOEnPer_V1.0/log/"); //输出日志目录
	google::SetStderrLogging(google::GLOG_WARNING);//级别高于WARNINGDE日志才有输出到屏幕，但若设置输出到控制台，该设置无效

	FLAGS_colorlogtostderr = true; //设置日志输出带颜色
	FLAGS_logbufsecs = 0; //日志实时输出
	FLAGS_max_log_size = 1024; //日志的最大大小，单位M

	#if 1
	LOG(INFO) << "---------- Init UART ----------";
	JetsonUart = jetsonSerial::getInstance();

	char *OpenFile_receive = "/dev/ttyTHS1";
	char *OpenFile_send = "/dev/ttyS0";
	
	
	if(!JetsonUart->Transceriver_UART_init(OpenFile_receive, 9600, 0, 8, 1, 'N',RECEIVER))  //configurate the Receive UART
	{
		LOG(ERROR) <<"Cannot Open the ttyTHS1";
		return -1;
	}
	

	if(!JetsonUart->Transceriver_UART_init(OpenFile_send, 9600, 0, 8, 1, 'N',SEND))  //configurate the Send UART
	{
		LOG(ERROR) <<"Cannot Open the ttyS0";
		return -1;
	}

	LOG(INFO) << "---------- Start Get data through UART ----------" ;
	uart_id = JetsonUart->startThread();

	#endif
	LOG(INFO) << "---------- InitCamera ----------" ;
	JetsonVideo = new VideoImg();
	if (!JetsonVideo->InitCamera(3280, 2464, 1280, 720, 20, 2))  // configurate the CSI camera stream
	{
		LOG(ERROR)<< "CANNOT INIT CAMERA";
		return -1;
	}

	LOG(INFO) << "---------- START for Work ----------" ;
	JetsonUart->Send_TriggerVoice(1,200);//Running the program will voice
	sleep(1);
	JetsonUart->Send_TriggerVoice(0);

	atexit(server_on_exit);
	signal(SIGINT,signal_exit_handler);  //Response the "kill the process"
	signal(SIGTERM,signal_exit_handler); //Response the "Ctrl+C end foreground process"


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
			LOG(INFO) << "---------- Start Prediction ----------";
			Classify_id = JetsonVideo->startThread_classify();
		}
   		if(cv::waitKey(30)>0)
		break;
	}
	pthread_cancel(uart_id);
	pthread_join(uart_id, NULL);
	pthread_exit(NULL);
	google::ShutdownGoogleLogging();
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
	LOG(INFO)<< "HAD SUCCESS EXIT";	
}

void signal_exit_handler(int sig)
{
	
	exit(0);
}

