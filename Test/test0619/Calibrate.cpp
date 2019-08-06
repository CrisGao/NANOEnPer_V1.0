#include "Calibrate.h"


using namespace cv;
using namespace std;
using namespace INMOTION;


void JetsonCalib::outputCameraParam(void)
{
	FileStorage fs("intrinsics.yml", FileStorage::WRITE);
	if (fs.isOpened())
	{
		fs << "cameraMatrix" << intrinsic_matrix << "cameraDistcoeffs" << distortion_coeffs;
		fs.release();
		cout << "cameraMatrix=:" << intrinsic_matrix << endl << "cameraDistcoeffs=:" << distortion_coeffs << endl ;
	}
	else
	{
		cout << "Error: can not save the intrinsics!!!!!" << endl;
	}


}



//int board_corner_cols = 11; int board_corner_rows = 8; int board_square = 32;
bool JetsonCalib::IMX219_Calibrate(int board_cols,int board_rows,int board_squares)
{

	int board_corner_cols = board_cols;
	int board_corner_rows = board_rows;
	int board_square = board_squares;

	Size board_size = Size(board_corner_cols, board_corner_rows);
	Size square_size = Size(board_square, board_square);

	int image_count = 17; //the nums of default calibration image is 17

	vector<Point2f> corners;//detect one image chesscorners
	vector<vector<Point2f>>  corners_Seq;// detect all images chesscorners
	vector<Mat>  image_Seq;
	int successImageNum = 0; // success calibrated images

	int count = 0;
	for (int i = 0; i != image_count; i++)
	{
		cout << "Frame #" << i << "..." << endl;
		string imageFileName;
		std::stringstream StrStm;
		StrStm << "./inter/" << i ;
		StrStm >> imageFileName;
		imageFileName += ".jpg";
		cv::Mat image = imread(imageFileName);


		Mat imageGray;
		cvtColor(image, imageGray, CV_RGB2GRAY);

		bool patternfound = findChessboardCorners(image, board_size, corners, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE +
			CALIB_CB_FAST_CHECK);
		if (!patternfound)
		{
			cout << "cannot find the image:" << imageFileName << endl;
			getchar();
			//exit(1);
			return false;
		}
		else
		{
	
			cornerSubPix(imageGray, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			Mat imageTemp = image.clone();
			cv::drawChessboardCorners(imageTemp, board_size, corners, patternfound);
			namedWindow(imageFileName, CV_WINDOW_NORMAL);
			imshow(imageFileName, imageTemp);
			waitKey(10);
			count = count + corners.size();
			successImageNum = successImageNum + 1;
			corners_Seq.push_back(corners);
			
		}
		image_Seq.push_back(image);
		
	}
	destroyAllWindows();


	vector<vector<Point3f>>  object_Points;        /****  保存定标板上角点的三维坐标   ****/
	vector<int>  point_counts;
	/* 定义棋盘格物理世界坐标 */
	for (int t = 0; t<successImageNum; t++)
	{
		vector<Point3f> tempPointSet;
		for (int i = 0; i<board_size.height; i++)
		{
			for (int j = 0; j<board_size.width; j++)
			{
				
				Point3f tempPoint;
				tempPoint.x = i*square_size.width;
				tempPoint.y = j*square_size.height;
				tempPoint.z = 0;
				tempPointSet.push_back(tempPoint);
			}
		}
		object_Points.push_back(tempPointSet);
	}
	for (int i = 0; i< successImageNum; i++)
	{
		point_counts.push_back(board_size.width*board_size.height);
	}

	Size image_size = image_Seq[0].size();
	std::vector<cv::Vec3d> rotation_vectors;                           /* 每幅图像的旋转向量 */
	std::vector<cv::Vec3d> translation_vectors;                        /* 每幅图像的平移向量 */
	cv::calibrateCamera(object_Points, corners_Seq, image_size, intrinsic_matrix, distortion_coeffs, rotation_vectors, translation_vectors,0);
	//int flags = 0;
	//flags |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
	//flags |= cv::fisheye::CALIB_CHECK_COND;
	//flags |= cv::fisheye::CALIB_FIX_SKEW;
	//fisheye::calibrate(object_Points, corners_Seq, image_size, intrinsic_matrix, distortion_coeffs, rotation_vectors, translation_vectors, flags, cv::TermCriteria(3, 20, 1e-6));


	/*结果评估*/
	double total_err = 0.0;                   /* 所有图像的平均误差的总和 */
	double err = 0.0;                        /* 每幅图像的平均误差 */
	vector<Point2f>  image_points2;             /****   保存重新计算得到的投影点    ****/

	for (int i = 0; i<image_count; i++)
	{
		vector<Point3f> tempPointSet = object_Points[i];
		projectPoints(tempPointSet, rotation_vectors[i], translation_vectors[i], intrinsic_matrix, distortion_coeffs, image_points2);
		vector<Point2f> tempImagePoint = corners_Seq[i];
		Mat tempImagePointMat = Mat(1, tempImagePoint.size(), CV_32FC2);
		Mat image_points2Mat = Mat(1, image_points2.size(), CV_32FC2);
		for (size_t i = 0; i != tempImagePoint.size(); i++)
		{
			image_points2Mat.at<Vec2f>(0, i) = Vec2f(image_points2[i].x, image_points2[i].y);
			tempImagePointMat.at<Vec2f>(0, i) = Vec2f(tempImagePoint[i].x, tempImagePoint[i].y);
		}
		err = norm(image_points2Mat, tempImagePointMat, NORM_L2);
		total_err += err /= point_counts[i];
	}
	cout << "Total error is:" << total_err / image_count << "pixel" << endl;

	/*保存参数*/
	outputCameraParam();

	/*矫正图片*/
	Mat mapx = Mat(image_size, CV_32FC1);
	Mat mapy = Mat(image_size, CV_32FC1);
	Mat R = Mat::eye(3, 3, CV_32F);


	Mat WaitForImg = imread("./inter/1.jpg");

	//fisheye::initUndistortRectifyMap(intrinsic_matrix,distortion_coeffs,R,intrinsic_matrix,image_size,CV_32FC1,mapx,mapy);
	Mat NiceImg, NiceImg2Gray;
	
	initUndistortRectifyMap(intrinsic_matrix, distortion_coeffs, R, intrinsic_matrix, image_size, CV_32FC1, mapx, mapy);
	cv::remap(WaitForImg, NiceImg, mapx, mapy, INTER_LINEAR);
	imwrite("fourPoint_d.png",NiceImg);
	return true;
}


