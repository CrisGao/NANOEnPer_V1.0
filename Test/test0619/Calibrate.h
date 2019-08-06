#include "opencv2/core/core.hpp"  
#include "opencv2/imgproc/imgproc.hpp"  
#include "opencv2/calib3d/calib3d.hpp"  
#include "opencv2/highgui/highgui.hpp"  
#include <iostream>  
#include <fstream>  


namespace INMOTION {
	class JetsonCalib
	{
	public:
		cv::Mat intrinsic_matrix;    /*****    摄像机内参数矩阵    ****/
		cv::Mat distortion_coeffs;     /* 摄像机的4个畸变系数：k1,k2,k3,k4*/
		bool IMX219_Calibrate(int board_cols, int board_rows, int board_squares);
	private:
		void outputCameraParam(void);


	};
	
}
