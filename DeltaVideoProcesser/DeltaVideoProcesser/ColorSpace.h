#pragma once

#include <windows.h>
#include "Helper.h"
#include "ColorReflection.h"
#include "RegressionGenerate.h"

#include "RegressionLeastSquare.h"
#include "RegressionElmLinear.h"

#define DECLARE_RUNTIME(class_name)	\
string class_name##Name;	\
static GenDynamic* class_name##GD

#define IMPLEMENT_RUNTIME(class_name)	\
GenDynamic* class_name::class_name##GD	\
= new GenDynamic(#class_name, class_name::CreateInstance);

class GenDynamic
{
public:
	GenDynamic(string name, CreateClass method)
	{
		ClassFactory::RegistClass(name, method);
	}
};

class CColorSpace
{
private:
	DECLARE_RUNTIME(CColorSpace);

	CRegressionGenerate		*m_pRegressionGenerate;

public:
	CColorSpace(void);
	virtual ~CColorSpace(void);

	static void* CreateInstance()
	{
		return new CColorSpace;
	}

	void	Initialize(int nChartNum, string RegressionAlg);

	void	GetCCMatrix(double (&CCMatrix)[3][3]);

	virtual BOOL CalibratedRGB(RGBTRIPLE *prgbIpCamRGB) { return FALSE; }			// [in, out] calibrated RGB

	virtual BOOL CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB) { return FALSE; }

protected:
	// Use "Matrix" to calculate a 3xN XYZ matrix for N color checker patch
	BOOL	RGBtoXYZ(	RGBTRIPLE	*prgbStandardRGB,		// A standard RGB array with N RGBTRIPLE items
						double		**ppMatrixStandardXYZ);	// Output 3xN standard XYZ matrix

	BOOL	RGBtoHSL(	RGBTRIPLE	*prgbStandardRGB,
						double		**ppMatrixStandardHSL);

	// For overall image frame
	BOOL	RGBtoXYZ(	RGBTRIPLE	*prgbIpCamRGB,			// IP cam RGBTRIPLE pixel
						double		(&MatrixIpCam)[3][3],	// Matrix that translate RGB to XYZ
						double		(&MatrixIpCamXYZ)[3]);	// Output IP cam XYZ

	BOOL	XYZtoLab(	double **ppMatrixXYZ,				// in
						double **ppMatrixLab);				// out

	BOOL	XYZtoLab(	double MatrixIpCamXYZ[3],
						double (&MatrixIpCamLab)[3]);		// Output IP cam Lab

	BOOL	XYZtoRGB(	double		(&MatrixIpCamXYZ)[3],	// IP cam XYZ
						RGBTRIPLE	*prgbIpCamRGB);			// Output calibrated RGB

	BOOL	LabtoXYZ(	double (&MatrixIpCamLab)[3],		// IP cam Lab
						double (&MatrixIpCamXYZ)[3]);		// IP cam XYZ

	BOOL	RGBtoHSL(	RGBTRIPLE	*prgbIpCamRGB,
						double		(&MatrixIpCamHSL)[3]);

	BOOL	HSLtoRGB(	double		(&MatrixIpCamHSL)[3],
						RGBTRIPLE	*prgbIpCamRGB);

protected:
	int		m_nStandardPatch;
	double	m_MatrixIpCam[3][3];
	CRegression *m_pRegressionAlg;
};

