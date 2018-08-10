#include "ColorGenerate.h"
#include "ColorReflection.h"


CColorGenerate::CColorGenerate(void)
{
}


CColorGenerate::~CColorGenerate(void)
{
}


CColorSpace* CColorGenerate::CreateColorObject(void)
{
	string ClassName = "CColorSpace" + m_strClassName;
	return (CColorSpace*)ClassFactory::GetClassByName(ClassName);
}
