#include "VideoImg.h"
#include <stdint.h>
#include <stdio.h>  /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

#include <queue>
#include <ctime>
#include <vector>

#include "utility.h"


extern double VehicleSpeed;

cv::Mat img;


pthread_mutex_t Img_mutex;
	
std::queue<float> que_score;

int Flags ;
int votes = 0;
bool trigger = false;

/******************/

int Frame_i = 0;
cv::FileStorage fs;
cv::VideoWriter writer;
/*****************/

int nums = 0;

/****************/
bool ifshow = false;
bool ifClone = true;

VideoImg::VideoImg()
{
	
}

VideoImg::~VideoImg()
{
	cap->release();
	pthread_mutex_destroy(&Img_mutex);
	pthread_exit(NULL);
	
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
	
	
	if (!cap->read(getImg))
	{
		std::cout << "Capture read error" << std::endl;
		return false;
	}


	if(ifClone)
	{
	ifClone =false;
	pthread_mutex_lock(&Img_mutex);
	img = getImg.clone();
	pthread_mutex_unlock(&Img_mutex);

	}

	return true;

}

bool VideoImg::ifGetImageFromCamera()
{
	if(!ifSTARTthread)
	{
	if(getImg.empty())   //if first time had no init camera,will still circulation
	{
		LOGG("non-image,is empty!!!!!!!!");
		return false;
	}
	else
	{
		LOGG("Start for saveVideo!!!!!!!!");
		ifSTARTthread = true;
		return true;
	}
	}
	else
	return false;
}


/******classification work***************************/

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

	/****init classify********/
#if 1
	std::cout << "---------- Loading model sysfile ----------" << std::endl;
	string model_file = "/home/leon/NANOEnPer_V1.0/data/deploy.prototxt";
	string trained_file = "/home/leon/NANOEnPer_V1.0/data/3caffe_train_iter_600000.caffemodel";
	string mean_file = "/home/leon/NANOEnPer_V1.0/data/mean.binaryproto";
	string label_file = "/home/leon/NANOEnPer_V1.0/data/label.txt";
	Classifier newClassf = Classifier(model_file, trained_file, mean_file, label_file); 
	LOGG("Finish init classfiy !!");
#endif
	/********initivate the uart class************/
	jetsonSerial *JetsonUartInClassify = jetsonSerial::getInstance();
	/*******************************************/

	Prediction maxS ;
	while (1)
	{
		
		if(img.empty())
		continue;
		cv::Mat Input_image;

		//pthread_cleanup_push(cleanup1, NULL);
		pthread_mutex_lock(&Img_mutex);
		Input_image = img;
		pthread_mutex_unlock(&Img_mutex);
		
		//pthread_cleanup_pop(0);

	CHECK(!Input_image.empty()) << "Unabel to decode image" << std::endl;

	std::vector<Prediction> predictions = newClassf.Classify(Input_image);

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

			maxS = PreS;
		}
	}
	std::cout << std::fixed << std::setprecision(4) << maxS.second << " - \"" << maxS.first << "\"" << std::endl;
#if 1
		if (QueuePrediction(maxS))
		{

			JetsonUartInClassify->Send_TriggerVoice(1);
			
			ifshow = true;
			
		}
		else
		{
			JetsonUartInClassify->Send_TriggerVoice(0);

			ifshow = false;

		}
#endif
		
		ifClone =true;
		
	}
	
}

 
bool QueuePrediction(Prediction maxS)
{

	if (maxS.first.compare("0 RoadWay") == 0)
	{
		Flags = 1;	//if roadway +1
		votes ++;
	}
	else
	Flags = 0;

	que_score.push(Flags);

	if( que_score.size() == 20)
	{
		if(votes > 10)
		{
			trigger = true;//trigger
			
		}
		else
			trigger = false;//non-trigger
		
		for(int i =0;i<20;i++)
		que_score.pop();
		votes = 0;
		
	}
	
	return trigger;
	
}

void VideoImg::deleteSources()
{

	
	pthread_mutex_destroy(&Img_mutex);


	LOGG("EXIT:had delete all sources");
	
}
/***********END***************************/
