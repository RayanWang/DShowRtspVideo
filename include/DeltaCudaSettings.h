#pragma once

// {46BEAE9E-3218-475C-B269-5DCCC350055C}
DEFINE_GUID(IID_IDeltaCudaSettings, 
0x46beae9e, 0x3218, 0x475c, 0xb2, 0x69, 0x5d, 0xcc, 0xc3, 0x50, 0x5, 0x5c);


[uuid("46BEAE9E-3218-475C-B269-5DCCC350055C")]
interface IDeltaCudaSettings : public IUnknown
{
	STDMETHOD_(DWORD, GetDeinterlaceMode)() = 0;
	STDMETHOD(SetDeinterlaceMode)(DWORD dwMode) = 0;

	STDMETHOD_(BOOL,GetFrameDoubling)() = 0;
	STDMETHOD(SetFrameDoubling)(BOOL bFrameDoubling) = 0;

	// Field Order - 0: Auto; 1: Top First; 2: Bottom First
	STDMETHOD_(DWORD, GetFieldOrder)() = 0;
	STDMETHOD(SetFieldOrder)(DWORD dwFieldOrder) = 0;

	STDMETHOD(GetFormatConfiguration)(bool *bFormat) = 0;
	STDMETHOD(SetFormatConfiguration)(bool *bFormat) = 0;

	STDMETHOD_(BOOL, GetStreamAR)() = 0;
	STDMETHOD(SetStreamAR)(BOOL bFlag) = 0;

	STDMETHOD_(BOOL, GetDXVA)() = 0;
	STDMETHOD(SetDXVA)(BOOL bDXVA) = 0;

	// Output Format - 0: Auto/Both; 1: NV12, 2: YV12
	STDMETHOD_(DWORD, GetOutputFormat)() = 0;
	STDMETHOD(SetOutputFormat)(DWORD dwOutput) = 0;
};
