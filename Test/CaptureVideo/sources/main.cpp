#include <fstream>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include  <dirent.h>
#include <opencv2/opencv.hpp>
 #include <opencv2/videoio.hpp>
 #include <opencv2/highgui.hpp>

#include "uart_configuration.h"

#define BYTES_OF_CURRENT_FOLDER 4096

std::string gstreamer_pipeline (int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method) {
    return "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(capture_width) + ", height=(int)" +
           std::to_string(capture_height) + ", format=(string)NV12, framerate=(fraction)" + std::to_string(framerate) +
           "/1 ! nvvidconv flip-method=" + std::to_string(flip_method) + " ! video/x-raw, width=(int)" + std::to_string(display_width) + ", height=(int)" +
           std::to_string(display_height) + ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}


class CheckSpace
{
public:

	//构造函数
	CheckSpace(char *filepath)
	{
		this -> m_TB = 0;
		this -> m_GB = 0;
		this -> m_MB = 0;
		this -> m_KB = 0;
		this -> m_Bytes = 0;
		strcpy(this -> m_FilePath, filepath);

		Check(filepath); //统计目录中的文件占据的空间大小
		AddBytes(4096);  //加上该目录本身占据的4096
	}

	//获取各项属性
	int GetTB() { return this -> m_TB; }
	int GetGB() { return this -> m_GB; }
	int GetMB() { return this -> m_MB; }
	int GetKB() { return this -> m_KB; }
	int GetBytes() { return this -> m_Bytes; }

	//展示内容
	void Display()
	{
		printf("查询目录路径 %s\n", m_FilePath);
		printf("占用空间 %dTB %dGB %dMB %dKB %dByte(s)\n",
			m_TB, m_GB, m_MB, m_KB, m_Bytes);
	}

private:

	int m_TB;    //TB
	int m_GB;    //GB
	int m_MB;    //MB
	int m_KB;    //KB
	int m_Bytes; //Byte
	char m_FilePath[128]; //目录地址

	//Byte数量增加（自动进位）
	void AddBytes(int bytes)
	{
		m_Bytes += bytes;
		while (m_Bytes >= 1024)
		{
			m_Bytes -= 1024;
			m_KB++;
		}
		while (m_KB >= 1024)
		{
			m_KB -= 1024;
			m_MB++;
		}
		while (m_MB >= 1024)
		{
			m_MB -= 1024;
			m_GB++;
		}
		while (m_GB >= 1024)
		{
			m_GB -= 1024;
			m_TB++;
		}
	}

	//查看某目录所占空间大小（不含该目录本身的4096Byte）
	void Check(char *dir)
	{
		DIR *dp;
		struct dirent *entry;
		struct stat statbuf;

		if ((dp = opendir(dir)) == NULL)
		{
			fprintf(stderr, "Cannot open dir: %s\n", dir);
			exit(0);
		}

		chdir(dir);

		while ((entry = readdir(dp)) != NULL)
		{
			lstat(entry -> d_name, &statbuf);
			if (S_ISDIR(statbuf.st_mode))
			{
				if (strcmp(".", entry -> d_name) == 0 ||
					strcmp("..", entry -> d_name) == 0)
				{
					continue;
				}

				AddBytes(statbuf.st_size);
				Check(entry -> d_name);
			}
			else
			{
				AddBytes(statbuf.st_size);
			}
		}

		chdir("..");
		closedir(dp);
	}
};

std::string currentTime()
{
	time_t rawtime;
	struct tm *ptminfo;
	char buf[128]={0};
	time(&rawtime);
	ptminfo = localtime(&rawtime);
	
	std::stringstream ss;
	std::string str;
	ss<<(ptminfo->tm_year+1900)<<(ptminfo->tm_mon+1)<<ptminfo->tm_mday<<ptminfo->tm_hour<<ptminfo->tm_min<<ptminfo->tm_sec;       
	ss>>str;
	return str;
}


int main()
{
	
#if 1
	std::cout << "---------- Init UART ----------" << std::endl;
	jetsonSerial* JetsonUart = jetsonSerial::getInstance();

	char *OpenFile_send = "/dev/ttyS0";

	if(!JetsonUart->Transceriver_UART_init(OpenFile_send, 9600, 0, 8, 1, 'N',SEND))  //configurate the Send UART
	{
		std::cout<<"Cannot Open the ttyS0"<<std::endl;
		return -1;
	}

	char topdir[100] = "/home/leon/CaptureVideo/video";
	CheckSpace cs = CheckSpace(topdir);
	if(cs.GetGB() >=40)
	{
	JetsonUart->Send_TriggerVoice(1);
	std::cout<<"The computer storge is not enough"<<std::endl;
	sleep(10);
	return -1;
	}
	
	std::cout << "---------- START for Work ----------" << std::endl;
	JetsonUart->Send_TriggerVoice(1);//Running the program will voice
	sleep(3);
	JetsonUart->Send_TriggerVoice(0);

#endif	

      int capture_width = 3280 ;
      int capture_height = 2464 ;
    //int capture_width = atoi(argv[1]);
   // int capture_height = atoi(argv[2]);
    int display_width = 1280;
    int display_height = 720;
    int framerate =20;
    int flip_method = 2;
 

    std::string pipeline = gstreamer_pipeline(capture_width,
	capture_height,
	display_width,
	display_height,
	framerate,
	flip_method);

    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);

    if(!cap.isOpened()) {
	std::cout<<"Failed to open camera."<<std::endl;
	return (-1);
    }

    cv::Mat img;
    cv::VideoWriter writer;
    std::string sFirstName = "/home/leon/CaptureVideo/video/"+currentTime();
    std::string sVideoName = sFirstName +".avi";
    writer.open(sVideoName,CV_FOURCC('M','J','P','G'),framerate,cv::Size(1280,720),1);


bool ifsave = false;
bool choose_20 = false;
bool isFull = false;

struct timeval tv;
gettimeofday(&tv, NULL);
int first_time = tv.tv_sec;
int n = 0;
    while(true)
    {
    	if (!cap.read(img)) {
		std::cout<<"Capture read error"<<std::endl;
		break;
	}
	
	//int keycode = cv::waitKey(30)&0xff;

	writer.write(img); 

	gettimeofday(&tv, NULL);
	int next_time = tv.tv_sec;
	int loop_time = next_time - first_time;

	if((!choose_20) && (loop_time == 1200))
	{
		ifsave = true;
		choose_20 = true;
		first_time = next_time;
		
	}
	else if(choose_20 && (loop_time == 900))
	{
		
		ifsave = true;
		first_time = next_time;
		
	}

	if(ifsave)
	{
	writer.release();
	ifsave = false;
	
	
	n ++;
	if(n<6 && n>0)
	{
	std::string sVideoName_1 = sFirstName +"_" + std::to_string(n)+".avi";
	writer.open(sVideoName_1,CV_FOURCC('M','J','P','G'),framerate,cv::Size(1280,720),1);
	}
	else if(n>6)
	break;
	}

       // if (keycode ==27) 
	//{
 	//break;
	//}

	

    }

    cap.release();

    return 0;
}

