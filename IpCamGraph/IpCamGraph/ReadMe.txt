========================================================================
    ACTIVE TEMPLATE LIBRARY：IpCamGraph 專案概觀
========================================================================

AppWizard 已為您建立了這個 IpCamGraph 專案，可以當做撰寫動態連結程式庫 (DLL) 的起點。

這個檔案包含一份摘要，簡要說明構成您的專案的所有檔案，它們個別的內容。

IpCamGraph.vcxproj
    這是使用應用程式精靈所產生之 VC++ 專案的主要專案檔。它含有產生該檔案之 Visual C++ 的版本資訊，以及有關使用應用程式精靈所選取之平台、組態和專案功能的資訊。

IpCamGraph.vcxproj.filters
    這是使用應用程式精靈所產生之 VC++ 專案的篩選檔。檔案中含有您專案中檔案與篩選器之間關聯的相關資訊。這項關聯用於 IDE 中以顯示特定節點下具有類似副檔名之檔案的群組 (例如，".cpp" 檔案會與 "Source Files" 篩選器相關聯)。

IpCamGraph.idl
    這個檔案包含型別程式庫的 IDL 定義，也就是定義在專案中的介面和共同類別。
    這個檔案將會由 MIDL 編譯器處理以產生：
        C++ 介面定義和 GUID 宣告 (IpCamGraph.h)
        GUID 定義                                (IpCamGraph_i.c)
        型別程式庫                                  (IpCamGraph.tlb)
        封送處理程式碼                                 (IpCamGraph_p.c 和 dlldata.c)

IpCamGraph.h
    這個檔案含有在  IpCamGraph.idl 中所定義之項目的 C++ 介面定義和 GUID 宣告。它將會由 MIDL 在編譯期間重新產生。

IpCamGraph.cpp
    這個檔案含有您 DLL 匯出的物件對應和實作。

IpCamGraph.rc
    這是程式所用的所有 Microsoft Windows 資源的列表。

IpCamGraph.def
    這個模組定義檔為連結器提供了有關您 DLL 所需之匯出的資訊。它包含下列項目的匯出：
        DllGetClassObject
        DllCanUnloadNow
        DllRegisterServer
        DllUnregisterServer
        DllInstall

/////////////////////////////////////////////////////////////////////////////
其他標準檔案：

StdAfx.h, StdAfx.cpp
    這些檔案是用來建置名為 IpCamGraph.pch 的先行編譯標頭 (PCH) 檔，以及名為 StdAfx.obj 的先行編譯型別檔。

Resource.h
    這是定義資源 ID 的標準標頭檔。

/////////////////////////////////////////////////////////////////////////////
Proxy/Stub DLL 專案和模組定義檔：

IpCamGraphps.vcxproj
    這個檔案是在必要時用來建置 Proxy/Stub DLL 的專案檔。
	主要專案中的 IDL 檔案必須至少包含一個介面，而且您必須先編譯 IDL 檔案才能建置 Proxy/Stub DLL。
	這個處理序會產生建置 Proxy/Stub DLL 時所需的 dlldata.c、IpCamGraph_i.c 和 IpCamGraph_p.c。

IpCamGraphps.vcxproj.filters
    這是 Proxy/Stub 專案的篩選檔。檔案中含有您專案中檔案與篩選器之間關聯的相關資訊。這項關聯用於 IDE 中以顯示特定節點下具有類似副檔名之檔案的群組 (例如，".cpp" 檔案會與 "Source Files" 篩選器相關聯)。

IpCamGraphps.def
    這個模組定義檔為連結器提供了有關 Proxy/Stub 所需之匯出的資訊。

/////////////////////////////////////////////////////////////////////////////
其他注意事項:


	[MFC 支援] 選項會將 MFC 程式庫建置到您的基本架構應用程式中，讓您能夠使用 MFC 的類別、物件和函式。
/////////////////////////////////////////////////////////////////////////////
