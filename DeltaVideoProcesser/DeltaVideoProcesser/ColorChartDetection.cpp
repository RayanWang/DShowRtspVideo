#include "ColorChartDetection.h"
#include "dlib\dlib\image_keypoint\surf.h"
#include "dlib\dlib\opencv\to_open_cv.h"

bool SortAccordingX(const ChartInfo lhs, const ChartInfo rhs)
{
	return lhs.nX < rhs.nX;
} 

bool SortAccordingY(const ChartInfo lhs, const ChartInfo rhs)
{
	return lhs.nY > rhs.nY;
}

ColorChartDetection::ColorChartDetection(void)
{
	// D:\\IVA Projects\\CameraCalibrationTool\\Debug
	std::string strFileName = "\\colorchecker_template.jpg";

	TCHAR szImagePath[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, szImagePath);
	int utf8Size = WideCharToMultiByte(CP_UTF8, 0, szImagePath, -1, NULL, 0, NULL, false);
	char* utf8Str = new char[utf8Size+1];
	WideCharToMultiByte(CP_UTF8, 0, szImagePath, -1, utf8Str, utf8Size, NULL, false);

	std::string strImagePath(utf8Str);
	strImagePath.append(strFileName);
	m_imgTemplate = imread(strImagePath); //目標樣本

	delete [] utf8Str;

	m_vColorPatch.clear();
	m_vOutputBounding.clear();
}

ColorChartDetection::~ColorChartDetection()
{
	m_vColorPatch.clear();
}

void ColorChartDetection::Init(Mat imgSrc)
{
	m_imgSrc = imgSrc;
}

BOOL ColorChartDetection::GetColorChartNum(int *pnPatchNum)
{
	if (m_imgTemplate.cols == NULL)
	{ 
		return FALSE;
	}
	if (m_imgSrc.cols == NULL)
	{ 
		return FALSE;
	}
	GetFeatursesPoints(m_imgTemplate, m_imgSrc);

	if (m_imgROI.cols == NULL)
	{ 
		return FALSE;
	}

	int blockW = 0;
	int blockH = 0;
	blockW = m_imgROI.cols / COLOR_PATCH_COL;
	blockH = m_imgROI.rows / COLOR_PATCH_ROW;
	Mat img_output;
	Vec3b vecPoint;
	ChartInfo chartInfo = {0};

	for (int k = 0; k < COLOR_PATCH_ROW; k++)
	{
		for (int j = 0; j < COLOR_PATCH_COL; j++)
		{
			img_output = m_imgROI(Rect(j*blockW, k*blockH, blockW, blockH));
			vecPoint = img_output.at<Vec3b>(img_output.cols / 2, img_output.rows / 2);

			chartInfo.rgbChart.rgbtRed		= vecPoint.val[2];
			chartInfo.rgbChart.rgbtGreen	= vecPoint.val[1];
			chartInfo.rgbChart.rgbtBlue		= vecPoint.val[0];

			m_vColorPatch.push_back(chartInfo);

			// Display ROI
			Rect rcOutputBounding;
			rcOutputBounding.x = j*blockW + m_rcRoiCorners.x;
			rcOutputBounding.y = k*blockH + m_rcRoiCorners.y;
			rcOutputBounding.width = blockW;
			rcOutputBounding.height = blockH;
			m_vOutputBounding.push_back(rcOutputBounding);
		}
	}
	if(pnPatchNum)
		*pnPatchNum = m_vColorPatch.size();

	return TRUE;
}

BOOL ColorChartDetection::GetColorChartValue(RGBTRIPLE **ppColorPatch)
{
	if(m_vColorPatch.size() <= 0)
		return FALSE;

	for(int i = 0; i < (int)m_vColorPatch.size(); ++i)
	{
		(*ppColorPatch)[i].rgbtBlue		= m_vColorPatch[i].rgbChart.rgbtBlue;
		(*ppColorPatch)[i].rgbtGreen	= m_vColorPatch[i].rgbChart.rgbtGreen;
		(*ppColorPatch)[i].rgbtRed		= m_vColorPatch[i].rgbChart.rgbtRed;
	}

	return TRUE;
}

vector<Rect> & ColorChartDetection::GetOutputBounding(void)
{
	return m_vOutputBounding;
}

Rect & ColorChartDetection::GetRoiRect(void)
{
	return m_rcRoiCorners;
}

// Private functions
void ColorChartDetection::GetFeatursesPoints(Mat img_template, Mat img_src)
{
	// Read pixel value
	Vec3b vecPoint;
	double brightness, object_total = 0, scene_total = 0;

	// SURF eigen point
	double minHessian = 350;

	for (int y = 0; y < img_template.rows; y++)
	{
		for (int x = 0; x < img_template.cols; x++)
		{
			vecPoint = img_template.at<Vec3b>(y, x);
			brightness = (vecPoint.val[0] + vecPoint.val[1] + vecPoint.val[2]) / 3;
			object_total = object_total + brightness;
		}
	}
	
	int size = img_template.rows * img_template.cols;
	object_total = object_total / size;
	brightness = 0;

	for (int y = 0; y < img_src.rows; y++)
	{
		for (int x = 0; x < img_src.cols; x++)
		{
			vecPoint = img_src.at<Vec3b>(y, x);
			brightness = (vecPoint.val[0] + vecPoint.val[1] + vecPoint.val[2]) / 3;
			scene_total = scene_total + brightness;
		}
	}

	size = img_src.rows * img_src.cols;
	scene_total = scene_total / size;

	if (scene_total < 70 || scene_total > 240)
	{ 
		throw "brightness error!";
		return;
	}
	//-- Step 1: Detect the keypoints using SURF Detector
	brightness = scene_total / object_total;
	if (brightness >= 0.85 && brightness <= 1.1)
	{
		minHessian = 400;
	}
	else if(brightness > 1.1 && brightness <= 1.8)
	{
		minHessian = 450;
	}
	else if (brightness >= 0.5 && brightness < 0.85)
	{
		minHessian = 350;
	}
	
	SurfFeatureDetector detector(minHessian);

	std::vector<KeyPoint> keypoints_object, keypoints_scene;

	detector.detect(img_template, keypoints_object);
	detector.detect(img_src, keypoints_scene);

	//-- Step 2: Calculate descriptors (feature vectors)
	SurfDescriptorExtractor extractor;

	Mat descriptors_object, descriptors_scene;

	extractor.compute(img_template, keypoints_object, descriptors_object);
	extractor.compute(img_src, keypoints_scene, descriptors_scene);

	//-- Step 3: Matching descriptor vectors using FLANN matcher
	FlannBasedMatcher matcher;
	std::vector< DMatch > matches;
	matcher.match(descriptors_object, descriptors_scene, matches);

	double max_dist = 0; 
	double min_dist = 100;

	//-- Quick calculation of max and min distances between keypoints
	for (int i = 0; i < descriptors_object.rows; i++)
	{
		double dist = matches[i].distance;
		if (dist < min_dist) 
			min_dist = dist;
		if (dist > max_dist) 
			max_dist = dist;
	}

	//-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
	std::vector< DMatch > good_matches;

	for (int i = 0; i < descriptors_object.rows; i++)
	{
		if (matches[i].distance < 3 * min_dist)
		{
			good_matches.push_back(matches[i]);
		}
	}

	//-- Localize the object
	std::vector<Point2f> obj;
	std::vector<Point2f> scene;
	
	for (int i = 0; i < (int)good_matches.size(); i++)
	{
		//-- Get the keypoints from the good matches
		obj.push_back(keypoints_object[good_matches[i].queryIdx].pt);
		scene.push_back(keypoints_scene[good_matches[i].trainIdx].pt);
	}

	Mat H = findHomography(obj, scene, CV_RANSAC);

	//-- Get the corners from the image_1 ( the object to be "detected" )
	std::vector<Point2f> obj_corners(4);
	obj_corners[0] = cvPoint(0, 0); 
	obj_corners[1] = cvPoint(img_template.cols, 0);
	obj_corners[2] = cvPoint(img_template.cols, img_template.rows); 
	obj_corners[3] = cvPoint(0, img_template.rows);

	std::vector<Point2f> scene_corners(4);

	perspectiveTransform(obj_corners, scene_corners, H);
	if (scene_corners[0].x == NULL)
	{ 
		throw "Can not found colorchart!";
		return;
	}

	// ROI Colorchart, need to be adjusted this parameter dynamically
	ZeroMemory(&m_rcRoiCorners, sizeof(Rect));
	m_rcRoiCorners.x = (int)min(scene_corners[0].x, scene_corners[3].x);
	m_rcRoiCorners.y = (int)min(scene_corners[0].y, scene_corners[1].y);
	m_rcRoiCorners.width = (int)max(scene_corners[1].x, scene_corners[2].x) - m_rcRoiCorners.x;
	m_rcRoiCorners.height = (int)max(scene_corners[2].y, scene_corners[3].y) - m_rcRoiCorners.y;
	/*m_rcRoiCorners.x = (int)scene_corners[0].x;
	m_rcRoiCorners.y = (int)scene_corners[0].y;
	
	int nWidth = 0, nHeight = 0;
	if (scene_corners[1].x - scene_corners[0].x > scene_corners[2].x - scene_corners[3].x)
	{
		nWidth = (int)(scene_corners[1].x - scene_corners[0].x);
	}
	else
	{
		nWidth = (int)(scene_corners[2].x - scene_corners[3].x);
	}
	if (scene_corners[3].y - scene_corners[0].y > scene_corners[2].y - scene_corners[1].y)
	{
		nHeight = (int)(scene_corners[3].y - scene_corners[0].y);
	}
	else
	{
		nHeight = (int)(scene_corners[2].y - scene_corners[1].y);
	}

	m_rcRoiCorners.width = nWidth;
	m_rcRoiCorners.height = nHeight;*/

	if(m_rcRoiCorners.width && m_rcRoiCorners.height)
		m_imgROI = img_src(Rect((int)scene_corners[0].x, (int)scene_corners[0].y, m_rcRoiCorners.width, m_rcRoiCorners.height));
}
