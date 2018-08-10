// dllmain.cpp : DllMain ªº¹ê§@¡C

#include "stdafx.h"
#include "resource.h"
#include "IpCamGraph_i.h"
#include "dllmain.h"

CIpCamGraphModule _AtlModule;

class CIpCamGraphApp : public CWinApp
{
public:

// ÂÐ¼g
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CIpCamGraphApp, CWinApp)
END_MESSAGE_MAP()

CIpCamGraphApp theApp;

BOOL CIpCamGraphApp::InitInstance()
{
	return CWinApp::InitInstance();
}

int CIpCamGraphApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
