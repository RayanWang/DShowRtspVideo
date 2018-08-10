#include "ColorSpace.h"
#include <math.h>
#include "ParamDef.h"

map<string, CreateRegressionClass> RegressionClassFactory::m_classMap;

IMPLEMENT_REGRESSION_RUNTIME(CRegression)
IMPLEMENT_REGRESSION_RUNTIME(CRegressionLeastSquare)
IMPLEMENT_REGRESSION_RUNTIME(CRegressionElmLinear)

CColorSpace::CColorSpace(void) 
{ 
	InitMatrix(m_MatrixIpCam); 
	m_pRegressionGenerate = new CRegressionGenerate;
}


CColorSpace::~CColorSpace(void) 
{
	SAFE_DELETE(m_pRegressionGenerate);
	SAFE_DELETE(m_pRegressionAlg);
}


void CColorSpace::Initialize(int nChartNum, string RegressionAlg)
{
	m_nStandardPatch = nChartNum;

	try
	{
		m_pRegressionAlg = m_pRegressionGenerate->CreateRegressionObject(RegressionAlg);
	}
	catch(...)
	{
		return;
	}
}


void CColorSpace::GetCCMatrix(double (&CCMatrix)[3][3])
{
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			CCMatrix[i][j] = m_MatrixIpCam[i][j];
		}
	}
}


BOOL CColorSpace::RGBtoXYZ(RGBTRIPLE *prgbStandardRGB, double **ppMatrixStandardXYZ)
{
	CheckPointer(prgbStandardRGB, FALSE);
	CheckPointer(ppMatrixStandardXYZ, FALSE);

	for(int i = 0; i < m_nStandardPatch; ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			ppMatrixStandardXYZ[j][i] = g_MatrixD65[j][0]*prgbStandardRGB[i].rgbtRed + 
										g_MatrixD65[j][1]*prgbStandardRGB[i].rgbtGreen + 
										g_MatrixD65[j][2]*prgbStandardRGB[i].rgbtBlue;
		}
	}

	// The standard XYZ value relative to the standard RGB value
	double dbX, dbY, dbZ;
	dbX = g_MatrixD65[0][0]*255 + g_MatrixD65[0][1]*255 + g_MatrixD65[0][2]*255;
	dbY = g_MatrixD65[1][0]*255 + g_MatrixD65[1][1]*255 + g_MatrixD65[1][2]*255;
	dbZ = g_MatrixD65[2][0]*255 + g_MatrixD65[2][1]*255 + g_MatrixD65[2][2]*255;

	// Normalize
	for(int i = 0; i < m_nStandardPatch; ++i)
	{
		ppMatrixStandardXYZ[0][i] /= dbX;
		ppMatrixStandardXYZ[1][i] /= dbY;
		ppMatrixStandardXYZ[2][i] /= dbZ;
	}

	return TRUE;
}


BOOL CColorSpace::RGBtoHSL(RGBTRIPLE *prgbStandardRGB, double **ppMatrixStandardHSL)
{
	CheckPointer(prgbStandardRGB, FALSE);
	CheckPointer(ppMatrixStandardHSL, FALSE);
	
	for(int i = 0; i < m_nStandardPatch; ++i)
	{
		double max = (double)max(prgbStandardRGB[i].rgbtRed, max(prgbStandardRGB[i].rgbtGreen, prgbStandardRGB[i].rgbtBlue));
		double min = (double)min(prgbStandardRGB[i].rgbtRed, min(prgbStandardRGB[i].rgbtGreen, prgbStandardRGB[i].rgbtBlue));

		double Diff = max - min;
		if(0 == Diff)
			ppMatrixStandardHSL[0][i] = 0;
		else if(prgbStandardRGB[i].rgbtRed == max)
			ppMatrixStandardHSL[0][i] = fmod(((double)(prgbStandardRGB[i].rgbtGreen-prgbStandardRGB[i].rgbtBlue)/Diff), 6.0);
		else if(prgbStandardRGB[i].rgbtGreen == max)
			ppMatrixStandardHSL[0][i] = (double)(prgbStandardRGB[i].rgbtBlue-prgbStandardRGB[i].rgbtRed)/Diff + 2.0;
		else
			ppMatrixStandardHSL[0][i] = (double)(prgbStandardRGB[i].rgbtRed-prgbStandardRGB[i].rgbtGreen)/Diff + 4.0;

		// H
		ppMatrixStandardHSL[0][i] *= 60;
		// L
		ppMatrixStandardHSL[2][i] = (max+min)/2.0;
		// S
		if(0 == ppMatrixStandardHSL[2][i] || 0 == Diff)
			ppMatrixStandardHSL[1][i] = 0;
		else if(ppMatrixStandardHSL[2][i] > 0 && ppMatrixStandardHSL[2][i] <= 1.0/2.0)
			ppMatrixStandardHSL[1][i] = (max-min)/(ppMatrixStandardHSL[2][i]*2);
		else
			ppMatrixStandardHSL[1][i] = (max-min)/(2-ppMatrixStandardHSL[2][i]*2);
	}

	return TRUE;
}


BOOL CColorSpace::RGBtoXYZ(RGBTRIPLE *prgbIpCamRGB, double (&MatrixIpCam)[3][3], double (&MatrixIpCamXYZ)[3])
{
	CheckPointer(prgbIpCamRGB, FALSE);

	MatrixIpCamXYZ[0] = MatrixIpCam[0][0]*prgbIpCamRGB->rgbtRed + 
						MatrixIpCam[0][1]*prgbIpCamRGB->rgbtGreen + 
						MatrixIpCam[0][2]*prgbIpCamRGB->rgbtBlue;
	MatrixIpCamXYZ[1] = MatrixIpCam[1][0]*prgbIpCamRGB->rgbtRed + 
						MatrixIpCam[1][1]*prgbIpCamRGB->rgbtGreen + 
						MatrixIpCam[1][2]*prgbIpCamRGB->rgbtBlue;
	MatrixIpCamXYZ[2] = MatrixIpCam[2][0]*prgbIpCamRGB->rgbtRed + 
						MatrixIpCam[2][1]*prgbIpCamRGB->rgbtGreen + 
						MatrixIpCam[2][2]*prgbIpCamRGB->rgbtBlue;

	return TRUE;
}


BOOL CColorSpace::XYZtoLab(double **ppMatrixXYZ, double **ppMatrixLab)
{
	CheckPointer(ppMatrixXYZ, FALSE);
	CheckPointer(ppMatrixLab, FALSE);

	for(int i = 0; i < m_nStandardPatch; ++i)
	{
		double dbAryft[3] = {0.0, 0.0, 0.0};	// {f(X), f(Y), f(Z)}
		
		for(int j = 0; j < 3; ++j)
		{
			if(ppMatrixXYZ[j][i] > pow(6.0/29.0, 3))
				dbAryft[j] = pow(ppMatrixXYZ[j][i], 1.0/3.0);
			else
				dbAryft[j] = 1.0/3.0*pow(29.0/6.0, 2)*ppMatrixXYZ[j][i] + 16.0/116.0;
		}
		
		// L*
		ppMatrixLab[0][i] = 116.0*dbAryft[1] - 16;
		// a*
		ppMatrixLab[1][i] = 500.0*(dbAryft[0] - dbAryft[1]);
		// b*
		ppMatrixLab[2][i] = 200.0*(dbAryft[1] - dbAryft[2]);
	}

	return TRUE;
}


BOOL CColorSpace::XYZtoLab(double MatrixIpCamXYZ[3], double (&MatrixIpCamLab)[3])
{
	double dbAryft[3] = {0.0, 0.0, 0.0};	// {f(X), f(Y), f(Z)}
		
	for(int j = 0; j < 3; ++j)
	{
		if(MatrixIpCamXYZ[j] > pow(6.0/29.0, 3))
			dbAryft[j] = pow(MatrixIpCamXYZ[j], 1.0/3.0);
		else
			dbAryft[j] = 1.0/3.0*pow(29.0/6.0, 2)*MatrixIpCamXYZ[j] + 16.0/116.0;
	}
		
	// L*
	MatrixIpCamLab[0] = 116.0*dbAryft[1] - 16;
	// a*
	MatrixIpCamLab[1] = 500.0*(dbAryft[0] - dbAryft[1]);
	// b*
	MatrixIpCamLab[2] = 200.0*(dbAryft[1] - dbAryft[2]);

	return TRUE;
}


BOOL CColorSpace::XYZtoRGB(double (&MatrixIpCamXYZ)[3], RGBTRIPLE *prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);

	double dbRed	= g_MatrixInverseD65[0][0]*MatrixIpCamXYZ[0] + 
					  g_MatrixInverseD65[0][1]*MatrixIpCamXYZ[1] + 
					  g_MatrixInverseD65[0][2]*MatrixIpCamXYZ[2];
	double dbGreen	= g_MatrixInverseD65[1][0]*MatrixIpCamXYZ[0] + 
					  g_MatrixInverseD65[1][1]*MatrixIpCamXYZ[1] + 
					  g_MatrixInverseD65[1][2]*MatrixIpCamXYZ[2];
	double dbBlue	= g_MatrixInverseD65[2][0]*MatrixIpCamXYZ[0] + 
					  g_MatrixInverseD65[2][1]*MatrixIpCamXYZ[1] + 
					  g_MatrixInverseD65[2][2]*MatrixIpCamXYZ[2];

	prgbIpCamRGB->rgbtRed	= (dbRed > 255.0) ? 255 : ((dbRed < 0.0) ? 0 : (BYTE)dbRed);
	prgbIpCamRGB->rgbtGreen = (dbGreen > 255.0) ? 255 : ((dbGreen < 0.0) ? 0 : (BYTE)dbGreen);
	prgbIpCamRGB->rgbtBlue	= (dbBlue > 255.0) ? 255 : ((dbBlue < 0.0) ? 0 : (BYTE)dbBlue);

	return TRUE;
}


BOOL CColorSpace::LabtoXYZ(double (&MatrixIpCamLab)[3], double (&MatrixIpCamXYZ)[3])
{
	double dbAryft[3] = {0.0, 0.0, 0.0};	// {fx, fy, fz}
		
	dbAryft[1] = (MatrixIpCamLab[0] + 16.0)/116.0;
	dbAryft[0] = dbAryft[1] + MatrixIpCamLab[1]/500.0;
	dbAryft[2] = dbAryft[1] - MatrixIpCamLab[2]/200.0;

	for(int i = 0; i < 3; ++i)
	{
		if(dbAryft[i] > 6.0/29.0)
			MatrixIpCamXYZ[i] = pow(dbAryft[i], 3);
		else
			MatrixIpCamXYZ[i] = (dbAryft[i] - 16.0/116.0)*3*pow(6.0/29.0, 2);
	}

	return TRUE;
}


BOOL CColorSpace::RGBtoHSL(RGBTRIPLE *prgbIpCamRGB, double (&MatrixIpCamHSL)[3])
{
	CheckPointer(prgbIpCamRGB, FALSE);
	
	double max = (double)max(prgbIpCamRGB->rgbtRed, max(prgbIpCamRGB->rgbtGreen, prgbIpCamRGB->rgbtBlue));
	double min = (double)min(prgbIpCamRGB->rgbtRed, min(prgbIpCamRGB->rgbtGreen, prgbIpCamRGB->rgbtBlue));

	double Diff = max - min;
	if(0 == Diff)
		MatrixIpCamHSL[0] = 0;
	else if(prgbIpCamRGB->rgbtRed == max)
		MatrixIpCamHSL[0] = fmod(((double)(prgbIpCamRGB->rgbtGreen-prgbIpCamRGB->rgbtBlue)/Diff), 6.0);
	else if(prgbIpCamRGB->rgbtGreen == max)
		MatrixIpCamHSL[0] = (double)(prgbIpCamRGB->rgbtBlue-prgbIpCamRGB->rgbtRed)/Diff + 2.0;
	else
		MatrixIpCamHSL[0] = (double)(prgbIpCamRGB->rgbtRed-prgbIpCamRGB->rgbtGreen)/Diff + 4.0;

	// H
	MatrixIpCamHSL[0] *= 60;
	// L
	MatrixIpCamHSL[2] = (max+min)/2.0;
	// S
	if(0 == MatrixIpCamHSL[2] || 0 == Diff)
		MatrixIpCamHSL[1] = 0;
	else if(MatrixIpCamHSL[2] > 0 && MatrixIpCamHSL[2] <= 1.0/2.0)
		MatrixIpCamHSL[1] = (max-min)/(MatrixIpCamHSL[2]*2);
	else
		MatrixIpCamHSL[1] = (max-min)/(2-MatrixIpCamHSL[2]*2);

	return TRUE;
}


BOOL CColorSpace::HSLtoRGB(double (&MatrixIpCamHSL)[3], RGBTRIPLE *prgbIpCamRGB)
{
	CheckPointer(prgbIpCamRGB, FALSE);

	double dbq, dbp, dbhk;
	double dbRGB[3] = {0.0, 0.0, 0.0};

	if(MatrixIpCamHSL[2] < 1.0/2.0)
		dbq = MatrixIpCamHSL[2] * (MatrixIpCamHSL[2] + MatrixIpCamHSL[1]);
	else
		dbq = MatrixIpCamHSL[2] + MatrixIpCamHSL[1] - (MatrixIpCamHSL[2]*MatrixIpCamHSL[1]);
	dbp = 2*MatrixIpCamHSL[2] - dbq;
	dbhk = MatrixIpCamHSL[0]/360;
	dbRGB[0] = dbhk + 1.0/3.0;
	dbRGB[1] = dbhk;
	dbRGB[2] = dbhk - 1.0/3.0;

	for(int i = 0; i < 3; ++i)
	{
		if(dbRGB[i] < 0)
			dbRGB[i] += 1.0;
		else if(dbRGB[i] > 1)
			dbRGB[i] -= 1.0;
	}

	for(int i = 0; i < 3; ++i)
	{
		if(dbRGB[i] < 1.0/6.0)
			dbRGB[i] = dbp + ((dbq-dbp)*6*dbRGB[i]);
		else if(dbRGB[i] < 1.0/2.0)
			dbRGB[i] = dbq;
		else if(dbRGB[i] < 2.0/3.0)
			dbRGB[i] = dbp + ((dbq-dbp)*6*(2.0/3.0-dbRGB[i]));
		else
			dbRGB[i] = dbp;
	}

	prgbIpCamRGB->rgbtRed = (dbRGB[0] > 255.0) ? 255 : ((dbRGB[0] < 0.0) ? 0 : (BYTE)dbRGB[0]);
	prgbIpCamRGB->rgbtGreen = (dbRGB[1] > 255.0) ? 255 : ((dbRGB[1] < 0.0) ? 0 : (BYTE)dbRGB[1]);
	prgbIpCamRGB->rgbtBlue = (dbRGB[2] > 255.0) ? 255 : ((dbRGB[2] < 0.0) ? 0 : (BYTE)dbRGB[2]);

	return TRUE;
}