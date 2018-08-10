#pragma once

#ifdef __cplusplus
extern "C" {
#endif

	// {C38F033B-03BE-451E-8B9F-98D15C069A28}
	DEFINE_GUID(IID_IRtspStreaming, 
	0xc38f033b, 0x3be, 0x451e, 0x8b, 0x9f, 0x98, 0xd1, 0x5c, 0x6, 0x9a, 0x28);


    DECLARE_INTERFACE_(IRtspStreaming, IUnknown)
    {
		STDMETHOD(SetStreamURL) (THIS_
					WCHAR*		pwszURL
                 ) PURE;

		STDMETHOD(GetStreamURL) (THIS_
					WCHAR*		pwszURL
                 ) PURE;

		STDMETHOD(SetResolution) (THIS_
					int			nWidth,
					int			nHeight
                 ) PURE;

		STDMETHOD(SetAccount) (THIS_
					WCHAR*		pwszAccount
                 ) PURE;

		STDMETHOD(SetPassword) (THIS_
					WCHAR*		pwszPass
                 ) PURE;
    };

#ifdef __cplusplus
}
#endif