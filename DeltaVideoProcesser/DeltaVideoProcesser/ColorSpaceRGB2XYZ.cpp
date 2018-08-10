#include "ColorSpaceRGB2XYZ.h"


CColorSpaceRGB2XYZ::CColorSpaceRGB2XYZ(void)
{
}


CColorSpaceRGB2XYZ::~CColorSpaceRGB2XYZ(void)
{
}


BOOL CColorSpaceRGB2XYZ::CalibratedRGB(RGBTRIPLE	*prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);

	double MatrixXYZ[3] = {0.0, 0.0, 0.0};

	RGBtoXYZ(prgbIpCamRGB, m_MatrixIpCam, MatrixXYZ);

	return XYZtoRGB(MatrixXYZ, prgbIpCamRGB);
}


BOOL CColorSpaceRGB2XYZ::CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB)
{
	CheckPointer(prgbPatchStandardRGB, FALSE);
	CheckPointer(prgbPatchIpCamRGB, FALSE);

	BOOL bResult = FALSE;

	double **ppStandardMatrix = new double*[3];		// A 3xN XYZ matrix
	InitMatrix(ppStandardMatrix, 3, m_nStandardPatch);

	bResult = RGBtoXYZ(prgbPatchStandardRGB, ppStandardMatrix);

	double **ppPatchIpCam = new double*[3];
	InitMatrix(ppPatchIpCam, 3, m_nStandardPatch);

	for(int j = 0; j < m_nStandardPatch; ++j)
	{
		ppPatchIpCam[0][j] = (double)prgbPatchIpCamRGB[j].rgbtRed/255;
		ppPatchIpCam[1][j] = (double)prgbPatchIpCamRGB[j].rgbtGreen/255;
		ppPatchIpCam[2][j] = (double)prgbPatchIpCamRGB[j].rgbtBlue/255;
	}

	bResult &= m_pRegressionAlg->GetCCMfromRegression(ppStandardMatrix, ppPatchIpCam, m_nStandardPatch, m_MatrixIpCam);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppStandardMatrix[i]);
	SAFE_DELETE_ARRAY(ppStandardMatrix);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppPatchIpCam[i]);
	SAFE_DELETE_ARRAY(ppPatchIpCam);

	return bResult;
}