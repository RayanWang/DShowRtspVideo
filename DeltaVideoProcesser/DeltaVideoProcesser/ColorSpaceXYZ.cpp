#include "ColorSpaceXYZ.h"
#include "ParamDef.h"


CColorSpaceXYZ::CColorSpaceXYZ(void)
{
}


CColorSpaceXYZ::~CColorSpaceXYZ(void)
{
}


BOOL CColorSpaceXYZ::CalibratedRGB(RGBTRIPLE *prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);

	BOOL bResult = FALSE;
	double MatrixIpCam[3] = {0.0, 0.0, 0.0};

	bResult = RGBtoXYZ(prgbIpCamRGB, g_MatrixD65, MatrixIpCam);

	for(int i = 0; i < 3; ++i)
	{
		MatrixIpCam[i] = m_MatrixIpCam[i][0]*MatrixIpCam[0] + 
						 m_MatrixIpCam[i][1]*MatrixIpCam[1] + 
						 m_MatrixIpCam[i][2]*MatrixIpCam[2];
	}

	bResult &= XYZtoRGB(MatrixIpCam, prgbIpCamRGB);

	return bResult;
}


BOOL CColorSpaceXYZ::CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB)
{
	CheckPointer(prgbPatchStandardRGB, FALSE);
	CheckPointer(prgbPatchIpCamRGB, FALSE);

	BOOL bResult = FALSE;
	double **ppStandardMatrix = new double*[3];		// A 3xN XYZ matrix
	InitMatrix(ppStandardMatrix, 3, m_nStandardPatch);

	bResult = RGBtoXYZ(prgbPatchStandardRGB, ppStandardMatrix);

	double **ppPatchIpCamXYZ = new double*[3];		// A 3xN XYZ matrix
	InitMatrix(ppPatchIpCamXYZ, 3, m_nStandardPatch);
	RGBtoXYZ(prgbPatchIpCamRGB, ppPatchIpCamXYZ);

	bResult &= m_pRegressionAlg->GetCCMfromRegression(ppStandardMatrix, ppPatchIpCamXYZ, m_nStandardPatch, m_MatrixIpCam);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppStandardMatrix[i]);
	SAFE_DELETE_ARRAY(ppStandardMatrix);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppPatchIpCamXYZ[i]);
	SAFE_DELETE_ARRAY(ppPatchIpCamXYZ);

	return bResult;
}