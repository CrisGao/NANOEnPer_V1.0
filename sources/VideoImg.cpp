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


//Classifier* newClassf;

/******************/
//std::vector<int> vec_score(20);
	
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
int num1 = 0;
int num2 = 0;
bool ifshow = false;
bool ifClone = true;

VideoImg::VideoImg()
{
	
}

VideoImg::~VideoImg()
{
	cap.release();
	pthread_mutex_destroy(&Img_mutex);
	pthread_exit(NULL);
	
}

bool VideoImg::InitCamera(int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method)
{

	pthread_mutex_init(&Img_mutex, NULL);

	pipeline = gstreamer_pipeline(capture_width, capture_height, display_width, display_height, framerate, flip_method);
	
	//cap = new cv::VideoCapture(pipeline, cv::CAP_GSTREAMER);
	
	cap.open("test2.avi");

	if (!cap.isOpened())
	{

		std::cout << "Failed to open camera." << std::endl;
		return false;
	}
	//cv::namedWindow("CIS" , cv::WINDOW_NORMAL);
	//cv::setWindowProperty("CSI" , cv::WND_PROP_FULLSCREEN,cv::WINDOW_FULLSCREEN);
	return true;
}

bool VideoImg::startCamera()
{
	
	//pthread_mutex_lock(&Img_mutex);
	if (!cap.read(getImg))
	{
		std::cout << "Capture read error" << std::endl;
		DEBUG("The classfiy nums is %d,the total frame is %d",num2,num1);
	}
	if(ifClone)
	{
	ifClone =false;
	img = getImg.clone();
	num2++;
	}

#if 1
	if(ifshow)
	{
	cv::putText(getImg,"ROADWAY!!!!",cv::Point(50,60),cv::FONT_HERSHEY_SIMPLEX,1,cv::Scalar(0,0,255),4,8);
	}
	cv::imshow("CSI",getImg);
	//pthread_mutex_unlock(&Img_mutex);
#endif
	num1++;
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
	string model_file = "../data/deploy.prototxt";
	string trained_file = "../data/3caffe_train_iter_600000.caffemodel";
	string mean_file = "../data/mean.binaryproto";
	string label_file = "../data/label.txt";
	Classifier newClassf = Classifier(model_file, trained_file, mean_file, label_file); 
#endif
	/********initivate the uart class************/
	jetsonSerial *JetsonUartInClassify = jetsonSerial::getInstance();
	/*******************************************/
	
	//cv::Mat Input_image = cv::imread("test.jpg",1);
	Prediction maxS ;
	while (1)
	{
		
		if(img.empty())
		continue;
		cv::Mat Input_image;

		
		//pthread_cleanup_push(cleanup1, NULL);
		//pthread_mutex_lock(&Img_mutex);
		Input_image = img;
		//pthread_mutex_unlock(&Img_mutex);
		
		//pthread_cleanup_pop(0);
		


	clock_t startTime,endTime;


	CHECK(!Input_image.empty()) << "Unabel to decode image" << std::endl;

startTime = clock();

	std::vector<Prediction> predictions = newClassf.Classify(Input_image);
endTime = clock();
std::cout<<"The run time is :"<<(double)(endTime-startTime)/CLOCKS_PER_SEC<<"s"<<std::endl;
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

#if 1
		//if (QueuePrediction(maxS))
		if(true)
		{
		//DEBUG("WARNING!!!!!!!!!!!!!!!!!");
	
		
			JetsonUartInClassify->Send_TriggerVoice(1);
			
			ifshow = true;
			
		}
		else
		{
			JetsonUartInClassify->Send_TriggerVoice(0);
			//LOGG("the nums of Queue is not enough or 20 frames less than 10 roadway");
			ifshow = false;

		}
#endif
		
		ifClone =true;
		//usleep(50000);
		
	}
	
}

/*****
*the voting rule is that if there are more than 10,will output 1,which is the trigger alarm signal
*return 0 express 20 frames had more than 10 frames predict is roadway,so need to trigger alarm
*return 1 express 20 frames had less than 10 frames predict is roadway,didn't need to trigger alarm
*return 2 express the vector hadn't enough frames,need 20 frames to vote
*****/
#if 0
bool VectorPrediction(Prediction maxS)
{
LOGG("had in PRE!!!");
	int Flags;
	if (maxS.first.compare("0 RoadWay") == 0)
	{
		Flags = 1;	//if roadway +1
		
	}
	else
	Flags = 0;

	vec_score.push_back(Flags);
DEBUG("the vote is %d",vec_score.size());
	if(vec_score.size() == 20)
	{
		std::vector<int>::iterator it;
		int vote =0;
		LOGG("Second in PRE!!!");
		for(it =vec_score.begin();it !=vec_score.end();it++ )
			vote += *it ;
		
		if(vote > 10)
		{
			trigger = true;//trigger
		}
		else
			trigger = false;//non-trigger
		
		vec_score.clear();
	}
	else
	trigger = false;

	return trigger;
	
}
#endif 
bool QueuePrediction(Prediction maxS)
{

	//Prediction maxS = GetPreScore_Max(input_img);

	//std::cout << std::fixed << std::setprecision(4) << maxS.second << " - \"" << maxS.first << "\"" << std::endl;

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
		DEBUG("the votes is %d",votes);
		if(votes > 10)
		{
			trigger = true;//trigger
			
		}
		else
			trigger = false;//non-trigger
		
		for(int i =0;i<20;i++)
		que_score.pop();
		votes = 0;
		//if(que_score.empty())
		//LOGG("CLEARING!!!");
	}
	
	return trigger;
	
}
#if 0
bool QueuePrediction1(Prediction maxS)
{

	//Prediction maxS = GetPreScore_Max(input_img);

	//std::cout << std::fixed << std::setprecision(4) << maxS.second << " - \"" << maxS.first << "\"" << std::endl;

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
		
		if(que_score.front() == 1)
		{
			votes -=1;
		}

		que_score.pop();
	}
	
	return trigger;
	
}


Prediction GetPreScore_Max(cv::Mat Input_img)
{

clock_t startTime,endTime;

	cv::Mat Input_image = Input_img;

	CHECK(!Input_image.empty()) << "Unabel to decode image" << std::endl;
startTime = clock();

	std::vector<Prediction> predictions = newClassf->Classify(Input_image);
endTime = clock();
std::cout<<"The run time is :"<<(double)(endTime-startTime)/CLOCKS_PER_SEC<<"s"<<std::endl;
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

			return PreS;
		}
	}
}

#endif
void VideoImg::deleteSources()
{

	
	pthread_mutex_destroy(&Img_mutex);


	LOGG("EXIT:had delete all sources");
	
}
/***********END***************************/
