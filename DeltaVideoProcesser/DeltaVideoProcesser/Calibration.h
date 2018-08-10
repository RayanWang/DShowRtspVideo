#pragma once

#include "ColorChartDetection.h"
#include "Helper.h"

typedef struct FrameData
{
	BYTE		*pFrame;
	int			nWidth;
	int			nHeight;
	FrameData()
	{
		pFrame = NULL;
	}
	~FrameData()
	{
		SAFE_DELETE_ARRAY(pFrame);
	}
} FrameData;

enum RGB_COLOR
{
	R = 0,
	G = 1,
	B = 2,
};

class CColorGenerate;
class CColorSpace;
class CCurveFitting;

class CCalibration
{
public:
	CCalibration(void);
	virtual ~CCalibration(void);

#pragma region FOR_CALIBRATED_CAMERA
	// pdwStandardRGB, pdwPatchIpCamRGB: N patches
	BOOL	CalibratedRGB(RGBTRIPLE	*prgbIpCamRGB);					// [in, out] calibrated RGB

	BOOL	CalculateCCMatrix(RGBTRIPLE	*prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB);
#pragma endregion

#pragma region COMMON_FUNCTIONS
	BOOL	GetColorChartNum( FrameData	*pFrameRGB,				// [in]	Frame information
							  string	RegressionAlg,
							  int		*pnPatchNum);			// [out] Number of color patch

	BOOL	GetColorPatchArry(	RGBTRIPLE	**pprgbDemodulateRGB,
								RGBTRIPLE	**pprgbIpCamRGB = NULL);	// [out] 3xN color patch array	

	BOOL	Demodulate(RGBTRIPLE *prgbIn, RGBTRIPLE **prgbOut);

	BOOL	Demodulate(RGBTRIPLE *prgb);

	BOOL	GetOutputBounding(vector<Rect> &vOutputBounding);

	BOOL	GetRoiRect(Rect &rcRoiRect);

	void	GetCCMatrix(double (&CCMatrix)[3][3]);

	void	GetPolynomialCoef(double (&dbAryPolyCoef)[3], int nChannel);

	BOOL	SetColorSpace(string name);
#pragma endregion

	BOOL	SaveImageToFile(FrameData *pFrameRGB, std::string strFileName);

private:
	// Heap object
	ColorChartDetection *m_pPatchDetect;
	CColorGenerate		*m_pColorGenerate;
	CColorSpace			*m_pColorSpace;
	CCurveFitting		*m_pCurveFit;

	int					m_nChartNum;

	// Array of Poly Coef
	double				m_dbAryPolyCoefR[4];
	double				m_dbAryPolyCoefG[4];
	double				m_dbAryPolyCoefB[4];
};

