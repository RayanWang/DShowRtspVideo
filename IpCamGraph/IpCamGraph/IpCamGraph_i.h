

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0595 */
/* at Tue Feb 24 10:43:08 2015
 */
/* Compiler settings for IpCamGraph.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0595 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __IpCamGraph_i_h__
#define __IpCamGraph_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IGraphAPI_FWD_DEFINED__
#define __IGraphAPI_FWD_DEFINED__
typedef interface IGraphAPI IGraphAPI;

#endif 	/* __IGraphAPI_FWD_DEFINED__ */


#ifndef __GraphAPI_FWD_DEFINED__
#define __GraphAPI_FWD_DEFINED__

#ifdef __cplusplus
typedef class GraphAPI GraphAPI;
#else
typedef struct GraphAPI GraphAPI;
#endif /* __cplusplus */

#endif 	/* __GraphAPI_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IGraphAPI_INTERFACE_DEFINED__
#define __IGraphAPI_INTERFACE_DEFINED__

/* interface IGraphAPI */
/* [unique][nonextensible][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_IGraphAPI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4578E130-06F8-4D64-823F-708743C0C24A")
    IGraphAPI : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateGraph( 
            /* [in] */ BOOL bIsRender,
            /* [in] */ BSTR bstrRtspUrl,
            /* [in] */ int nColorSpace) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetDisplayWindow( 
            /* [in] */ LONG_PTR lWindow) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetNotifyWindow( 
            /* [in] */ LONG_PTR lWindow) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HandleEvent( 
            /* [in] */ UINT_PTR inWParam,
            /* [in] */ LONG_PTR inLParam) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Run( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitCalibrate( 
            /* [in] */ BOOL bIsStandardCamera) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableCalibrate( 
            /* [in] */ BOOL bEnable) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableDemodulate( 
            /* [in] */ BOOL bEnable) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnInitialize( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DestroyGraph( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableGPUCompute( 
            /* [in] */ BOOL bEnable) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SnapShot( 
            /* [in] */ BSTR bstrImgPath) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IGraphAPIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGraphAPI * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGraphAPI * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGraphAPI * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IGraphAPI * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateGraph )( 
            IGraphAPI * This,
            /* [in] */ BOOL bIsRender,
            /* [in] */ BSTR bstrRtspUrl,
            /* [in] */ int nColorSpace);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetDisplayWindow )( 
            IGraphAPI * This,
            /* [in] */ LONG_PTR lWindow);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWindow )( 
            IGraphAPI * This,
            /* [in] */ LONG_PTR lWindow);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HandleEvent )( 
            IGraphAPI * This,
            /* [in] */ UINT_PTR inWParam,
            /* [in] */ LONG_PTR inLParam);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Run )( 
            IGraphAPI * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IGraphAPI * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IGraphAPI * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InitCalibrate )( 
            IGraphAPI * This,
            /* [in] */ BOOL bIsStandardCamera);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EnableCalibrate )( 
            IGraphAPI * This,
            /* [in] */ BOOL bEnable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EnableDemodulate )( 
            IGraphAPI * This,
            /* [in] */ BOOL bEnable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UnInitialize )( 
            IGraphAPI * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DestroyGraph )( 
            IGraphAPI * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EnableGPUCompute )( 
            IGraphAPI * This,
            /* [in] */ BOOL bEnable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SnapShot )( 
            IGraphAPI * This,
            /* [in] */ BSTR bstrImgPath);
        
        END_INTERFACE
    } IGraphAPIVtbl;

    interface IGraphAPI
    {
        CONST_VTBL struct IGraphAPIVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGraphAPI_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IGraphAPI_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IGraphAPI_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IGraphAPI_Initialize(This)	\
    ( (This)->lpVtbl -> Initialize(This) ) 

#define IGraphAPI_CreateGraph(This,bIsRender,bstrRtspUrl,nColorSpace)	\
    ( (This)->lpVtbl -> CreateGraph(This,bIsRender,bstrRtspUrl,nColorSpace) ) 

#define IGraphAPI_SetDisplayWindow(This,lWindow)	\
    ( (This)->lpVtbl -> SetDisplayWindow(This,lWindow) ) 

#define IGraphAPI_SetNotifyWindow(This,lWindow)	\
    ( (This)->lpVtbl -> SetNotifyWindow(This,lWindow) ) 

#define IGraphAPI_HandleEvent(This,inWParam,inLParam)	\
    ( (This)->lpVtbl -> HandleEvent(This,inWParam,inLParam) ) 

#define IGraphAPI_Run(This)	\
    ( (This)->lpVtbl -> Run(This) ) 

#define IGraphAPI_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#define IGraphAPI_Pause(This)	\
    ( (This)->lpVtbl -> Pause(This) ) 

#define IGraphAPI_InitCalibrate(This,bIsStandardCamera)	\
    ( (This)->lpVtbl -> InitCalibrate(This,bIsStandardCamera) ) 

#define IGraphAPI_EnableCalibrate(This,bEnable)	\
    ( (This)->lpVtbl -> EnableCalibrate(This,bEnable) ) 

#define IGraphAPI_EnableDemodulate(This,bEnable)	\
    ( (This)->lpVtbl -> EnableDemodulate(This,bEnable) ) 

#define IGraphAPI_UnInitialize(This)	\
    ( (This)->lpVtbl -> UnInitialize(This) ) 

#define IGraphAPI_DestroyGraph(This)	\
    ( (This)->lpVtbl -> DestroyGraph(This) ) 

#define IGraphAPI_EnableGPUCompute(This,bEnable)	\
    ( (This)->lpVtbl -> EnableGPUCompute(This,bEnable) ) 

#define IGraphAPI_SnapShot(This,bstrImgPath)	\
    ( (This)->lpVtbl -> SnapShot(This,bstrImgPath) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IGraphAPI_INTERFACE_DEFINED__ */



#ifndef __IpCamGraphLib_LIBRARY_DEFINED__
#define __IpCamGraphLib_LIBRARY_DEFINED__

/* library IpCamGraphLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_IpCamGraphLib;

EXTERN_C const CLSID CLSID_GraphAPI;

#ifdef __cplusplus

class DECLSPEC_UUID("8EA1761B-9E81-4643-B110-DA8C1A78EF0B")
GraphAPI;
#endif
#endif /* __IpCamGraphLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


