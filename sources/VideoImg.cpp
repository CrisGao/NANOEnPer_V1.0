#include "VideoImg.h"
#include <stdint.h>
#include <stdio.h>  /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

#include <queue>

#include "utility.h"

 
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

extern double VehicleSpeed;

cv::Mat img;

pthread_mutex_t Img_mutex;

pthread_t Buzzer_thread_id;

Classifier *newClassf;

/******************/
int isRoadSide;
int gpio = 79;

/******************/
std::queue<float> vec_score;

int Flags ;
int votes = 0;
bool trigger = false;

/******************/

int Frame_i = 0;
cv::FileStorage fs;
cv::VideoWriter writer;
/*****************/

VideoImg::VideoImg()
{
	
}

VideoImg::~VideoImg()
{
	cap->release();
	pthread_exit(NULL);
	pthread_mutex_destroy(&Img_mutex);
}

bool VideoImg::InitCamera(int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method)
{

	pthread_mutex_init(&Img_mutex, NULL);

	pipeline = gstreamer_pipeline(capture_width, capture_height, display_width, display_height, framerate, flip_method);

	cap = new cv::VideoCapture(pipeline, cv::CAP_GSTREAMER);

	if (!cap->isOpened())
	{

		std::cout << "Failed to open camera." << std::endl;
		return false;
	}

	return true;
}

bool VideoImg::startCamera()
{

	pthread_mutex_lock(&Img_mutex);
	if (!cap->read(img))
	{
		std::cout << "Capture read error" << std::endl;
		return false;
	}

	pthread_mutex_unlock(&Img_mutex);

	return true;
}

/***********Setting Buzzer thread***************************/

pthread_t VideoImg::startThread_saveVideoSpeed()
{

	if (pthread_create(&VideoImg_thread_id, NULL, WriteVideo_Speed, NULL))
	{
		std::cout << "VideoImg_thread create error!" << std::endl;
		return -1;
	}
	else
	{
		std::cout << "VideoImg_thread create success!" << std::endl;
		return VideoImg_thread_id;
	}
}

void VideoImg::Init_VideoWriteFileStorage(int save_Width,int save_Height)
{
	std::string Videoname, VideoSpeedFilename;

	Videoname = "../video/" + currentTime() + ".avi";
	std::cout << "Videoname:" << Videoname << std::endl;

	VideoSpeedFilename = "../Speeds/" + currentTime() + ".yml";
	std::cout << "Save Speed YML in :" << VideoSpeedFilename << std::endl;

	fs = cv::FileStorage(VideoSpeedFilename, cv::FileStorage::WRITE);

        writer = cv::VideoWriter(Videoname, CV_FOURCC('M', 'J', 'P', 'G'), 20, cv::Size(save_Width, save_Height), 1);
}

void *WriteVideo_Speed(void *ptr)
{
	std::string imageFileName;

	while (1)
	{
		if(img.empty())   //if first time had no init camera,will still circulation
		{
			std::cout<<"non-image,is empty!!!!!!!!"<<std::endl;
			continue;
		}


		pthread_cleanup_push(cleanup, NULL);
		pthread_mutex_lock(&Img_mutex);
		writer.write(img);
		pthread_mutex_unlock(&Img_mutex);
		pthread_cleanup_pop(0);

		if (fs.isOpened())
		{
			imageFileName = "Frame" + std::to_string(Frame_i);

			std::cout << "save sequences:" << imageFileName << ",speed:" << VehicleSpeed << std::endl;

			fs << imageFileName << VehicleSpeed;
		}
		else
		{
			std::cout << "Error: can not save the VehicleSpeedParams!!!!!" << std::endl;
			break;
		}

		Frame_i++;

	}
	
	
}

void cleanup(void *arg)
{
	pthread_mutex_unlock(&Img_mutex);
}

void cleanup1(void *arg)
{
	pthread_mutex_unlock(&Img_mutex);
}
/***********END***************************/

/***********Setting Buzzer thread***************************/

int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
 
	return 0;
}

int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}
 
	if (out_flag)
	{
		write(fd, "out", 4);

	}
	else
		write(fd, "in", 3);
 
	close(fd);
	return 0;
}

int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}
 
	if (value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);
 
	close(fd);
	return 0;
}

void *play_buzzer(void *ptr)
{

	gpio_set_value(gpio,1);
	usleep(50000);
	printf("the time had out\n");
	gpio_set_value(gpio,0);
}

/****************END**************/

/******classification work***************************/

void VideoImg::Init_Classify(string model_file,string trained_file,string mean_file,string label_file)
{
	newClassf = new Classifier(model_file, trained_file, mean_file, label_file); 
	/****init GPIO********/
	//gpio_export(gpio);
	//gpio_set_dir(gpio, 1);
}

pthread_t VideoImg::startThread_classify()
{
	
	if (pthread_create(&Classify_thread_id, NULL, Classify_Work, NULL))
	{
		std::cout << "Classify_thread create error!" << std::endl;
		return -1;
	}
	else
	{
		std::cout << "Classify_thread create success!" << std::endl;
		return Classify_thread_id;
	}
}

void *Classify_Work(void *ptr)
{


	/********initivate the uart class************/
	jetsonSerial *JetsonUartInClassify = jetsonSerial::getInstance();
	/*******************************************/
	
	while (1)
	{
		if(img.empty())   //if first time had no init camera,will still circulation
		continue;

		cv::Mat input_image;
		cv::Mat ResizeImg;
		std::string currentTimeStr;

		pthread_cleanup_push(cleanup1, NULL);
		pthread_mutex_lock(&Img_mutex);
		input_image = img.clone();
		pthread_mutex_unlock(&Img_mutex);
		pthread_cleanup_pop(0);

		if (QueuePrediction(input_image))
		{
		
		#if 0
			if (pthread_create(&Buzzer_thread_id, NULL, play_buzzer, NULL))
			{
				std::cout << "Buzzer_thread create error!" << std::endl;
				break;
				
			}
			else
			{
				std::cout << "Buzzer_thread create success!" << std::endl;
				
			}
			
		#endif
		#if 1
			JetsonUartInClassify->Send_TriggerVoice(1);
		#endif
		}
		else
		{
			JetsonUartInClassify->Send_TriggerVoice(0);
			std::cout<<"the nums of Queue is not enough or 20 frames less than 10 roadway"<<std::endl;
		}
	

		usleep(50000);
	}
	
}

/*****
*the voting rule is that if there are more than 10,will output 1,which is the trigger alarm signal
*return 0 express 20 frames had more than 10 frames predict is roadway,so need to trigger alarm
*return 1 express 20 frames had less than 10 frames predict is roadway,didn't need to trigger alarm
*return 2 express the vector hadn't enough frames,need 20 frames to vote
*****/
bool QueuePrediction(cv::Mat input_img)
{

	Prediction maxS = GetPreScore_Max(input_img);

	std::cout << std::fixed << std::setprecision(4) << maxS.second << " - \"" << maxS.first << "\"" << std::endl;

	if (maxS.first.compare("0 RoadWay") == 0)
	{
		Flags = 1;	//if roadway +1
		votes ++;
	}
	else
	Flags = 0;

	vec_score.push(Flags);

	if( vec_score.size() == 20)
	{
		if(!vec_score.empty())
		{
			std::cout<<"votes's nums is :"<<votes<<std::endl;
			
			if(votes > 10)
			{
				trigger = true;//trigger
			}
			else
				trigger = false;//non-trigger
		}
		else
		std::cout<<"Cannot prediction the image's score"<<std::endl;

		if(vec_score.back() == 1)
		{
			votes -=1;
		}

		vec_score.pop();
	}
	
	return trigger;
	
}


Prediction GetPreScore_Max(cv::Mat Input_img)
{

	cv::Mat Input_image = Input_img;

	CHECK(!Input_image.empty()) << "Unabel to decode image" << std::endl;

	std::vector<Prediction> predictions = newClassf->Classify(Input_image);

	vector<double> Pscore;

	for (size_t i = 0; i < predictions.size(); ++i)
	{
		Prediction p = predictions[i];
		Pscore.push_back(p.second);
	}

	auto PreScore = max_element(Pscore.begin(), Pscore.end());

	for (size_t i = 0; i < predictions.size(); ++i)
	{
		Prediction PreS = predictions[i];
		if (*PreScore == PreS.second)
		{
			//std::cout<<std::fixed<<std::setprecision(4)<<PreS.second<<" - \""<<PreS.first<< "\""<<std::endl;
			return PreS;
		}
	}
}


void VideoImg::deleteSources()
{
	Frame_i = 0;
	fs.release();
	writer.release();

	delete newClassf;

	newClassf = NULL; //释放申请的对象内存和指向
	
	pthread_mutex_destroy(&Img_mutex);

	//gpio_unexport(gpio);//relase the GPIO79

	std::cout<<"had delete all sources"<<std::endl;
}
/***********END***************************/
