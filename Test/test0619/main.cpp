#include <fstream>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <opencv2/opencv.hpp>
 #include <opencv2/videoio.hpp>
 #include <opencv2/highgui.hpp>
#include "Calibrate.h"

using namespace INMOTION;

std::string gstreamer_pipeline (int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method) {
    return "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(capture_width) + ", height=(int)" +
           std::to_string(capture_height) + ", format=(string)NV12, framerate=(fraction)" + std::to_string(framerate) +
           "/1 ! nvvidconv flip-method=" + std::to_string(flip_method) + " ! video/x-raw, width=(int)" + std::to_string(display_width) + ", height=(int)" +
           std::to_string(display_height) + ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}

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




int main(int argc,char** argv)
{
	if(argc!=1)
	{
		std::cout<<"error!usage:simple_camera.out capture_width capture_height framerate flip_method\n";
		return -1;
	}
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
    std::cout << "Using pipeline: \n\t" << pipeline << "\n";
 
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
	//cv::VideoCapture cap("test3.mp4");
    if(!cap.isOpened()) {
	std::cout<<"Failed to open camera."<<std::endl;
	return (-1);
    }

    cv::namedWindow("CSI Camera", cv::WINDOW_NORMAL);
    cv::setWindowProperty("CSI Camera",cv::WND_PROP_FULLSCREEN,cv::WINDOW_FULLSCREEN);
    cv::Mat img;
    std::string sCalibImgName,sImageName,sVideoName;
    int nums = 0;
    bool ifSave = false;
    JetsonCalib* jetsonCal = new JetsonCalib();
    cv::VideoWriter writer;
    std::cout << "Hit 'c' to save CalibratedImage\n's' to save CaptureImage\n'g' to start Calibration\n'v' to save Video\n'ESC' to exit" << "\n" ;
    while(true)
    {
    	if (!cap.read(img)) {
		std::cout<<"Capture read error"<<std::endl;
		break;
	}
	
	int keycode = cv::waitKey(30) & 0xff;
         cv::imshow("CSI Camera",img); 
	
	if(keycode==99)//key is c
	{
	
		if(nums<17)
		{
	sCalibImgName ="../calibrateImg/"+std::to_string(nums)+".jpg";
	std::cout<<"save Calibrated Image"<<sCalibImgName<<std::endl;
	cv::imwrite(sCalibImgName,img);
	nums++;
		}
		else
		{
			nums =0;
		}

	}
       if(keycode==115)//key is s
       {
	       sImageName ="../saveImg/"+ currentTime()+".jpg";
	       std::cout<<"save CurrentImage:"<<sImageName<<std::endl;
	       cv::imwrite(sImageName,img);
	       
       }
  	if(keycode == 118)//key is v
	{
		sVideoName = "../video/"+currentTime()+".avi";
		if(!writer.isOpened())
		{  
		       	 writer =cv::VideoWriter(sVideoName,CV_FOURCC('M','J','P','G'),framerate,cv::Size(1280,720),1);
        		 ifSave = true;
			 std::cout<<"save Video:"<<sVideoName<<std::endl;
		}
		else
		{
			ifSave = false;
			std::cout<<"close Video"<<std::endl;
			writer.release();
		}
		
	}
	if(ifSave)
	{
       		 writer.write(img); 
	}
	
       if(keycode==103)//key is g,start calibrate normal camera,not eye camera
       {
	       int board_corner_cols = 11;
	       int board_corner_rows = 8;
	       int board_square = 32;
	       bool isOK = jetsonCal->IMX219_Calibrate(board_corner_cols,board_corner_rows,board_square);
	       if(isOK)
	       {
		       std::cout<<"Calibration had done!"<<std::endl;
	       }

       }
        if (keycode ==27) 
	{
 	break;
	}
	
    }

    cap.release();
    cv::destroyAllWindows() ;
    return 0;
}

