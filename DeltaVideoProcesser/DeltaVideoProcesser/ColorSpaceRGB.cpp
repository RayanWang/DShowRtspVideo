#include "ColorSpaceRGB.h"


CColorSpaceRGB::CColorSpaceRGB(void)
{
}


CColorSpaceRGB::~CColorSpaceRGB(void)
{
}


BOOL CColorSpaceRGB::CalibratedRGB(RGBTRIPLE *prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);

	double MatrixIpCam[3] = {0.0, 0.0, 0.0};

	for(int i = 0; i < 3; ++i)
	{
		MatrixIpCam[i] = m_MatrixIpCam[i][0]*prgbIpCamRGB->rgbtRed + 
						 m_MatrixIpCam[i][1]*prgbIpCamRGB->rgbtGreen +
						 m_MatrixIpCam[i][2]*prgbIpCamRGB->rgbtBlue;
	}

	prgbIpCamRGB->rgbtRed	= (MatrixIpCam[0] > 255.0) ? 255 : ((MatrixIpCam[0] < 0.0) ? 0 : (BYTE)MatrixIpCam[0]);
	prgbIpCamRGB->rgbtGreen = (MatrixIpCam[1] > 255.0) ? 255 : ((MatrixIpCam[0] < 0.0) ? 0 : (BYTE)MatrixIpCam[1]);
	prgbIpCamRGB->rgbtBlue	= (MatrixIpCam[2] > 255.0) ? 255 : ((MatrixIpCam[0] < 0.0) ? 0 : (BYTE)MatrixIpCam[2]);

	return TRUE;
}


BOOL CColorSpaceRGB::CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB)
{
	CheckPointer(prgbPatchStandardRGB, FALSE);
	CheckPointer(prgbPatchIpCamRGB, FALSE);

	BOOL bResult = FALSE;
	double **ppStandardMatrix = new double*[3];		// A 3xN XYZ matrix
	InitMatrix(ppStandardMatrix, 3, m_nStandardPatch);

	for(int j = 0; j < m_nStandardPatch; ++j)
	{
		ppStandardMatrix[0][j] = (double)prgbPatchStandardRGB[j].rgbtRed/255;
		ppStandardMatrix[1][j] = (double)prgbPatchStandardRGB[j].rgbtGreen/255;
		ppStandardMatrix[2][j] = (double)prgbPatchStandardRGB[j].rgbtBlue/255;
	}

	double **ppPatchIpCam = new double*[3];
	InitMatrix(ppPatchIpCam, 3, m_nStandardPatch);

	for(int j = 0; j < m_nStandardPatch; ++j)
	{
		ppPatchIpCam[0][j] = (double)prgbPatchIpCamRGB[j].rgbtRed/255;
		ppPatchIpCam[1][j] = (double)prgbPatchIpCamRGB[j].rgbtGreen/255;
		ppPatchIpCam[2][j] = (double)prgbPatchIpCamRGB[j].rgbtBlue/255;
	}

	bResult = m_pRegressionAlg->GetCCMfromRegression(ppStandardMatrix, ppPatchIpCam, m_nStandardPatch, m_MatrixIpCam);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppStandardMatrix[i]);
	SAFE_DELETE_ARRAY(ppStandardMatrix);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppPatchIpCam[i]);
	SAFE_DELETE_ARRAY(ppPatchIpCam);

	return bResult;
}
