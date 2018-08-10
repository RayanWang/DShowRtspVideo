#include "ColorSpaceHSL.h"
#include "ParamDef.h"


CColorSpaceHSL::CColorSpaceHSL(void)
{
}


CColorSpaceHSL::~CColorSpaceHSL(void)
{
}


BOOL CColorSpaceHSL::CalibratedRGB(RGBTRIPLE *prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);

	BOOL bResult = FALSE;

	double MatrixIpCamHSL[3] = {0.0, 0.0, 0.0};

	bResult = RGBtoHSL(prgbIpCamRGB, MatrixIpCamHSL);

	for(int i = 0; i < 3; ++i)
	{
		MatrixIpCamHSL[i] = m_MatrixIpCam[i][0]*MatrixIpCamHSL[0] + 
							m_MatrixIpCam[i][1]*MatrixIpCamHSL[1] + 
							m_MatrixIpCam[i][2]*MatrixIpCamHSL[2];
	}

	bResult &= HSLtoRGB(MatrixIpCamHSL, prgbIpCamRGB);

	return bResult;
}


BOOL CColorSpaceHSL::CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB)
{
	CheckPointer(prgbPatchStandardRGB, FALSE);
	CheckPointer(prgbPatchIpCamRGB, FALSE);

	BOOL bResult = FALSE;

	double **ppStandardMatrix = new double*[3];
	InitMatrix(ppStandardMatrix, 3, m_nStandardPatch);
	bResult = RGBtoHSL(prgbPatchStandardRGB, ppStandardMatrix);

	double **ppPatchIpCamHSL = new double*[3];
	InitMatrix(ppPatchIpCamHSL, 3, m_nStandardPatch);
	bResult &= RGBtoHSL(prgbPatchIpCamRGB, ppPatchIpCamHSL);

	bResult &= m_pRegressionAlg->GetCCMfromRegression(ppStandardMatrix, ppPatchIpCamHSL, m_nStandardPatch, m_MatrixIpCam);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppStandardMatrix[i]);
	SAFE_DELETE_ARRAY(ppStandardMatrix);
	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppPatchIpCamHSL[i]);
	SAFE_DELETE_ARRAY(ppPatchIpCamHSL);

	return bResult;
}
