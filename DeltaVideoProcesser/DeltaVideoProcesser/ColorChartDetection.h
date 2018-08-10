#pragma once

#include "core/core.hpp"
#include "features2d/features2d.hpp"
#include "highgui/highgui.hpp"
#include "calib3d/calib3d.hpp"
#include "nonfree/nonfree.hpp"
#include "nonfree/features2d.hpp"
#include "imgproc/imgproc.hpp"

#include <windows.h>
#include <vector>

#define MAX_CHART_NUM	24
#define COLOR_PATCH_COL	6
#define COLOR_PATCH_ROW	4

using namespace cv;

typedef struct ChartInfo
{
	RGBTRIPLE	rgbChart;
	int			nX;
	int			nY;
} ChartInfo;

class ColorChartDetection
{
public:
	ColorChartDetection(void);

	virtual ~ColorChartDetection();

	void Init(Mat imgSrc);

	BOOL GetColorChartNum(int *pnPatchNum);

	BOOL GetColorChartValue(RGBTRIPLE **ppColorPatch);

	vector<Rect> &GetOutputBounding(void);

	Rect &GetRoiRect(void);

private:
	// Recognition and detection
	void GetFeatursesPoints(Mat img_template, Mat img_src);

	Mat					m_imgSrc;
	Mat					m_imgTemplate;
	Mat					m_imgROI;
	vector<Rect>		m_vOutputBounding;
	vector<ChartInfo>	m_vColorPatch;

	Rect				m_rcRoiCorners;
};

