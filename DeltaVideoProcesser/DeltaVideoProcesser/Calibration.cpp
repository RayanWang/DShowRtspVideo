#include "Calibration.h"

#include "CurveFitting.h"

#include "ColorSpaceRGB.h"
#include "ColorSpaceRGB2XYZ.h"
#include "ColorSpaceXYZ.h"
#include "ColorSpaceLab.h"
#include "ColorSpaceHSL.h"

#include "ColorGenerate.h"

#include <math.h>
#include "ParamDef.h"

map<string, CreateClass> ClassFactory::m_classMap;

IMPLEMENT_RUNTIME(CColorSpace)
IMPLEMENT_RUNTIME(CColorSpaceRGB)
IMPLEMENT_RUNTIME(CColorSpaceRGB2XYZ)
IMPLEMENT_RUNTIME(CColorSpaceXYZ)
IMPLEMENT_RUNTIME(CColorSpaceLab)
IMPLEMENT_RUNTIME(CColorSpaceHSL)

double g_dbStandardGrayX[6] = {0.0, 0.20784, 0.45882, 0.70588, 0.97647, 1.0};

double g_dbDemodulateTableR[256];
double g_dbDemodulateTableG[256];
double g_dbDemodulateTableB[256];

double Polynomial(double dbX, double *pdbPolyCoef, int nExp)
{
	double Xn = 1;	// x^0

	double dbTotal = 0;
	for(int i = 0; i <= nExp; ++i)
	{
		dbTotal += pdbPolyCoef[i]*Xn;
		Xn *= dbX;
	}

	return dbTotal;
}


CCalibration::CCalibration(void)
	: m_pPatchDetect(new ColorChartDetection)
	, m_pColorGenerate(new CColorGenerate)
	, m_pColorSpace(NULL)
	, m_pCurveFit(new CCurveFitting)
	, m_nChartNum(0)
#ifdef _SIGMOID_FUNC_TEST
	, m_bTestFlag(FALSE)
#endif
{
	ZeroMemory(m_dbAryPolyCoefR, sizeof(m_dbAryPolyCoefR));
	ZeroMemory(m_dbAryPolyCoefG, sizeof(m_dbAryPolyCoefG));
	ZeroMemory(m_dbAryPolyCoefB, sizeof(m_dbAryPolyCoefB));

	ZeroMemory(g_dbDemodulateTableR, sizeof(g_dbDemodulateTableR));
	ZeroMemory(g_dbDemodulateTableG, sizeof(g_dbDemodulateTableG));
	ZeroMemory(g_dbDemodulateTableB, sizeof(g_dbDemodulateTableB));
}


CCalibration::~CCalibration(void)
{
	SAFE_DELETE(m_pPatchDetect);
	SAFE_DELETE(m_pColorGenerate);
	SAFE_DELETE(m_pColorSpace);
	SAFE_DELETE(m_pCurveFit);
}


// FOR_CALIBRATED_CAMERA
BOOL CCalibration::CalibratedRGB(RGBTRIPLE	*prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);
	CheckPointer(m_pColorSpace, FALSE);

	return m_pColorSpace->CalibratedRGB(prgbIpCamRGB);
}


BOOL CCalibration::CalculateCCMatrix(RGBTRIPLE	*prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB)
{
	CheckPointer(prgbPatchStandardRGB, FALSE);
	CheckPointer(prgbPatchIpCamRGB, FALSE);
	CheckPointer(m_pColorSpace, FALSE);

	BOOL bResult = FALSE;
	bResult = m_pColorSpace->CalculateCCMatrix(prgbPatchStandardRGB, prgbPatchIpCamRGB);

	return bResult;
}


// COMMON_FUNCTIONS
BOOL CCalibration::GetColorChartNum(FrameData	*pFrameRGB, 
									string		RegressionAlg,
									int			*pnPatchNum)
{
	CheckPointer(pFrameRGB, FALSE);
	CheckPointer(pnPatchNum, FALSE);
	CheckPointer(m_pColorGenerate, FALSE);

	// Find the RGB Patch through openCV lib
	cv::Mat inputImg(pFrameRGB->nHeight, pFrameRGB->nWidth, CV_8UC3, pFrameRGB->pFrame, pFrameRGB->nWidth*3);
	cv::Mat imgFlip;
	cv::flip(inputImg, imgFlip, 1);
	m_pPatchDetect->Init(imgFlip);

	m_pPatchDetect->GetColorChartNum(pnPatchNum);

	m_nChartNum = *pnPatchNum;
	if((*pnPatchNum ? FALSE : TRUE))
		return FALSE;

	SAFE_DELETE(m_pColorSpace);

	try
	{
		m_pColorSpace = m_pColorGenerate->CreateColorObject();
	}
	catch(...)
	{
		return FALSE;
	}

	m_pColorSpace->Initialize(*pnPatchNum, RegressionAlg);

	return TRUE;
}


BOOL CCalibration::GetColorPatchArry(RGBTRIPLE **pprgbDemodulateRGB, RGBTRIPLE **pprgbIpCamRGB)
{
	CheckPointer(pprgbDemodulateRGB, FALSE);
	CheckPointer(*pprgbDemodulateRGB, FALSE);
	CheckPointer(m_pPatchDetect, FALSE);

	BOOL bResult = FALSE;
	RGBTRIPLE *prgbChart = new RGBTRIPLE[m_nChartNum];

	bResult = m_pPatchDetect->GetColorChartValue(&prgbChart);
	bResult &= Demodulate(prgbChart, pprgbDemodulateRGB);

	if(pprgbIpCamRGB)
		memcpy(*pprgbIpCamRGB, prgbChart, m_nChartNum*3);

	SAFE_DELETE_ARRAY(prgbChart);

	return bResult;
}


BOOL CCalibration::Demodulate(RGBTRIPLE *prgbIn, RGBTRIPLE **prgbOut)
{
	CheckPointer(prgbIn, FALSE);
	CheckPointer(prgbOut, FALSE);
	CheckPointer(*prgbOut, FALSE);
	CheckPointer(m_pCurveFit, FALSE);

	int nXYpairs = MAX_CHART_NUM/4;
	double *pdbCapturedRedY = new double[nXYpairs];
	double *pdbCapturedGreenY = new double[nXYpairs];
	double *pdbCapturedBlueY = new double[nXYpairs];

	for(int i = 1; i <= nXYpairs; ++i)
	{
		pdbCapturedRedY[i-1]	= (double)(prgbIn[m_nChartNum-i].rgbtRed) / 255;
		pdbCapturedGreenY[i-1]	= (double)(prgbIn[m_nChartNum-i].rgbtGreen) / 255;
		pdbCapturedBlueY[i-1]	= (double)(prgbIn[m_nChartNum-i].rgbtBlue) / 255;
	}

	int nExp = sizeof(m_dbAryPolyCoefR)/sizeof(double) - 1;
	m_pCurveFit->polyfit(nXYpairs, g_dbStandardGrayX, pdbCapturedRedY, nExp, m_dbAryPolyCoefR);
	m_pCurveFit->polyfit(nXYpairs, g_dbStandardGrayX, pdbCapturedGreenY, nExp, m_dbAryPolyCoefG);
	m_pCurveFit->polyfit(nXYpairs, g_dbStandardGrayX, pdbCapturedBlueY, nExp, m_dbAryPolyCoefB);

	SAFE_DELETE_ARRAY(pdbCapturedRedY);
	SAFE_DELETE_ARRAY(pdbCapturedGreenY);
	SAFE_DELETE_ARRAY(pdbCapturedBlueY);

	for(int i = 1; i < 256; ++i)
	{
		double dbNormalizeValue = (double)(i)/255;
		double dbProduct = (double)(i) * dbNormalizeValue;
		g_dbDemodulateTableR[i] = dbProduct / Polynomial(dbNormalizeValue, m_dbAryPolyCoefR, nExp);
		g_dbDemodulateTableG[i] = dbProduct / Polynomial(dbNormalizeValue, m_dbAryPolyCoefG, nExp);
		g_dbDemodulateTableB[i] = dbProduct / Polynomial(dbNormalizeValue, m_dbAryPolyCoefB, nExp);
	}

	for(int i = 0; i < m_nChartNum; ++i)
	{
		double dbRed = g_dbDemodulateTableR[prgbIn[i].rgbtRed];
		double dbGreen = g_dbDemodulateTableG[prgbIn[i].rgbtGreen];
		double dbBlue = g_dbDemodulateTableB[prgbIn[i].rgbtBlue];

		(*prgbOut)[i].rgbtRed	= (dbRed > 255.0) ? 255 : ((dbRed < 0.0) ? 0 : (BYTE)dbRed);
		(*prgbOut)[i].rgbtGreen = (dbGreen > 255.0) ? 255 : ((dbGreen < 0.0) ? 0 : (BYTE)dbGreen);
		(*prgbOut)[i].rgbtBlue	= (dbBlue > 255.0) ? 255 : ((dbBlue < 0.0) ? 0 : (BYTE)dbBlue);
	}

	return TRUE;
}


BOOL CCalibration::Demodulate(RGBTRIPLE *prgb)
{
	int nExp = sizeof(m_dbAryPolyCoefR)/sizeof(double) - 1;

	double dbRed = g_dbDemodulateTableR[prgb->rgbtRed];
	double dbGreen = g_dbDemodulateTableG[prgb->rgbtGreen];
	double dbBlue = g_dbDemodulateTableB[prgb->rgbtBlue];

	prgb->rgbtRed	= (dbRed > 255.0) ? 255 : ((dbRed < 0.0) ? 0 : (BYTE)dbRed);
	prgb->rgbtGreen = (dbGreen > 255.0) ? 255 : ((dbGreen < 0.0) ? 0 : (BYTE)dbGreen);
	prgb->rgbtBlue	= (dbBlue > 255.0) ? 255 : ((dbBlue < 0.0) ? 0 : (BYTE)dbBlue);

	return TRUE;
}


BOOL CCalibration::GetOutputBounding(vector<Rect> &vOutputBounding)
{
	CheckPointer(m_pPatchDetect, FALSE);
	
	vOutputBounding = m_pPatchDetect->GetOutputBounding();

	return TRUE;
}


BOOL CCalibration::GetRoiRect(Rect &rcRoiRect)
{
	CheckPointer(m_pPatchDetect, FALSE);
	
	rcRoiRect = m_pPatchDetect->GetRoiRect();

	return TRUE;
}


void CCalibration::GetCCMatrix(double (&CCMatrix)[3][3])
{
	if(m_pColorSpace)
		m_pColorSpace->GetCCMatrix(CCMatrix);
}


void CCalibration::GetPolynomialCoef(double (&dbAryPolyCoef)[3], int nChannel)
{
	for(int i = 0; i < 3; ++i)
	{
		if(R == nChannel)
			dbAryPolyCoef[i] = m_dbAryPolyCoefR[i];
		else if(G == nChannel)
			dbAryPolyCoef[i] = m_dbAryPolyCoefG[i];
		else
			dbAryPolyCoef[i] = m_dbAryPolyCoefB[i];
	}
}


BOOL CCalibration::SetColorSpace(string name)
{
	CheckPointer(m_pColorGenerate, FALSE);

	m_pColorGenerate->SetColorSpace(name);

	return TRUE;
}


BOOL CCalibration::SaveImageToFile(FrameData *pFrameRGB, std::string strFileName)
{
	cv::Mat inputImg(pFrameRGB->nHeight, pFrameRGB->nWidth, CV_8UC3, pFrameRGB->pFrame, pFrameRGB->nWidth*3);
	cv::Mat imgFlip;
	cv::flip(inputImg, imgFlip, 1);
	return cv::imwrite(strFileName, imgFlip);
}