// dllmain.h : 模組類別的宣告。

class CIpCamGraphModule : public ATL::CAtlDllModuleT< CIpCamGraphModule >
{
public :
	DECLARE_LIBID(LIBID_IpCamGraphLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_IPCAMGRAPH, "{86D0CF1F-057C-45EC-8DAE-8ED560402656}")
};

extern class CIpCamGraphModule _AtlModule;
