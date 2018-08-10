#include "RegressionGenerate.h"
#include "RegressionReflection.h"


CRegressionGenerate::CRegressionGenerate(void)
{
}


CRegressionGenerate::~CRegressionGenerate(void)
{
}


CRegression* CRegressionGenerate::CreateRegressionObject(string Alg)
{
	string ClassName = "CRegression" + Alg;
	return (CRegression*)RegressionClassFactory::GetClassByName(ClassName);
}
