#pragma once

#include <windows.h>
#include "Helper.h"
#include "RegressionReflection.h"

#define DECLARE_REGRESSION_RUNTIME(class_name)	\
string class_name##Name;	\
static GenRegressionDynamic* class_name##GRD

#define IMPLEMENT_REGRESSION_RUNTIME(class_name)	\
GenRegressionDynamic* class_name::class_name##GRD	\
= new GenRegressionDynamic(#class_name, class_name::CreateInstance);

class GenRegressionDynamic
{
public:
	GenRegressionDynamic(string name, CreateRegressionClass method)
	{
		RegressionClassFactory::RegistClass(name, method);
	}
};

class CRegression
{
private:
	DECLARE_REGRESSION_RUNTIME(CRegression);

public:
	CRegression(void);
	virtual ~CRegression(void);

	static void* CreateInstance()
	{
		return new CRegression;
	}

	virtual BOOL GetCCMfromRegression(double **ppStandardMatrix,		// 3xN standard certain color space matrix
									  double **ppPatchIpCam,
									  int	 nStandardPatch,
									  double (&MatrixIpCam)[3][3]) 		// Output matrix
	{ 
		return FALSE; 
	}
};

