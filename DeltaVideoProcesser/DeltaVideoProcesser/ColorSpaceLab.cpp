#include "ColorSpaceLab.h"
#include "ParamDef.h"


CColorSpaceLab::CColorSpaceLab(void)
{
}


CColorSpaceLab::~CColorSpaceLab(void)
{
}


BOOL CColorSpaceLab::CalibratedRGB(RGBTRIPLE *prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);

	BOOL bResult = FALSE;
	double MatrixIpCam[3] = {0.0, 0.0, 0.0};

	bResult = RGBtoXYZ(prgbIpCamRGB, g_MatrixD65, MatrixIpCam);

	double MatrixIpCamLab[3] = {0.0f, 0.0f, 0.0f};
	bResult &= XYZtoLab(MatrixIpCam, MatrixIpCamLab);

	for(int i = 0; i < 3; ++i)
	{
		MatrixIpCamLab[i] = m_MatrixIpCam[i][0]*MatrixIpCamLab[0] +
							m_MatrixIpCam[i][1]*MatrixIpCamLab[1] + 
							m_MatrixIpCam[i][2]*MatrixIpCamLab[2];
	}

	bResult &= LabtoXYZ(MatrixIpCamLab, MatrixIpCam);

	bResult &= XYZtoRGB(MatrixIpCam, prgbIpCamRGB);

	return bResult;
}


BOOL CColorSpaceLab::CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB)
{
	CheckPointer(prgbPatchStandardRGB, FALSE);
	CheckPointer(prgbPatchIpCamRGB, FALSE);

	BOOL bResult = FALSE;
	double **ppStandardMatrix = new double*[3];		// A 3xN XYZ matrix
	InitMatrix(ppStandardMatrix, 3, m_nStandardPatch);

	bResult = RGBtoXYZ(prgbPatchStandardRGB, ppStandardMatrix);

	double **ppStandardLab = new double*[3];		// A 3xN Lab matrix
	InitMatrix(ppStandardLab, 3, m_nStandardPatch);
	bResult &= XYZtoLab(ppStandardMatrix, ppStandardLab);

	double **ppPatchIpCamXYZ = new double*[3];		// A 3xN XYZ matrix
	InitMatrix(ppPatchIpCamXYZ, 3, m_nStandardPatch);
	RGBtoXYZ(prgbPatchIpCamRGB, ppPatchIpCamXYZ);

	double **ppPatchIpCamLab = new double*[3];		// A 3xN Lab matrix
	InitMatrix(ppPatchIpCamLab, 3, m_nStandardPatch);
	XYZtoLab(ppPatchIpCamXYZ, ppPatchIpCamLab);

	bResult &= m_pRegressionAlg->GetCCMfromRegression(ppStandardLab, ppPatchIpCamLab, m_nStandardPatch, m_MatrixIpCam);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppStandardLab[i]);
	SAFE_DELETE_ARRAY(ppStandardLab);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppStandardMatrix[i]);
	SAFE_DELETE_ARRAY(ppStandardMatrix);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppPatchIpCamLab[i]);
	SAFE_DELETE_ARRAY(ppPatchIpCamLab);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppPatchIpCamXYZ[i]);
	SAFE_DELETE_ARRAY(ppPatchIpCamXYZ);

	return bResult;
}