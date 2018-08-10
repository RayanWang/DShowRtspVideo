#define EC_RTSPNOTIFY EC_USER + 0x4000
#define EC_RTSP_PARAM1_DISKFULL 1
#define EC_RTSP_PARAM1_DEVICELOST_RECONNECTING 3
#define EC_RTSP_PARAM1_DEVICELOST_RECONNECTED 4
#define EC_RTSP_PARAM1_RECORDTONEWFILE_COMPLETED 5
#define EC_RTSP_PARAM1_OPENURLASYNC_CONNECTION_RESULT 6
#define EC_RTSP_PARAM1_FRAME_CAPTURE_SUCCEEDED 7
#define EC_RTSP_PARAM1_FRAME_CAPTURE_FAILED 8

//_________________________________________________________________________________
// {55D1139D-5E0D-4123-9AED-575D7B039569}
static const GUID CLSID_DatasteadRtspRtmpSource = 
{ 0x55D1139D, 0x5E0D, 0x4123, { 0x9A, 0xED, 0x57, 0x5D, 0x7B, 0x03, 0x95, 0x69 } };

//_________________________________________________________________________________
// {D7557B82-3FA4-4F4F-B7CF-96108202E4AF}
static const GUID IID_IDatasteadRTSPSourceConfig = 
{ 0xd7557b82, 0x3fa4, 0x4f4f, { 0xb7, 0xcf, 0x96, 0x10, 0x82, 0x2, 0xe4, 0xaf } };

MIDL_INTERFACE("D7557B82-3FA4-4F4F-B7CF-96108202E4AF")
IDatasteadRTSPSourceConfig : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetBool   (LPCOLESTR ParamID,      bool  *Value) = 0; 
    virtual HRESULT STDMETHODCALLTYPE GetDouble (LPCOLESTR ParamID,    double  *Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetInt    (LPCOLESTR ParamID,       int  *Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStr    (LPCOLESTR ParamID,  LPOLESTR  *Value) = 0; 

    virtual HRESULT STDMETHODCALLTYPE SetBool   (LPCOLESTR ParamID,       bool  Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDouble (LPCOLESTR ParamID,     double  Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetInt    (LPCOLESTR ParamID,        int  Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStr    (LPCOLESTR ParamID,  LPCOLESTR  Value) = 0;

    virtual HRESULT STDMETHODCALLTYPE Action    (LPCOLESTR ActionID, LPCOLESTR Option) = 0;
};


// {58D66E77-B19C-4F9A-8B47-DFE1CA87B642}
static const GUID IID_IDatasteadRTSPSourceCallbackConfig = 
{ 0x58d66e77, 0xb19c, 0x4f9a, { 0x8b, 0x47, 0xdf, 0xe1, 0xca, 0x87, 0xb6, 0x42 } };

typedef void (__stdcall *OpenURLAsyncCompletionCB) (void *Sender, intptr_t CustomParam, HRESULT Result); 

MIDL_INTERFACE("58D66E77-B19C-4F9A-8B47-DFE1CA87B642")
IDatasteadRTSPSourceCallbackConfig : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetAsyncOpenURLCallback (OpenURLAsyncCompletionCB CallbackFunctionPtr, void *Sender, intptr_t CustomParam) = 0;
};

//_________________________________________________________________________________
// {799485B3-0DA1-4F74-90E2-5684C9CD949B}
DEFINE_GUID(IID_IDatasteadRTSPSampleCallback, 
0x799485b3, 0xda1, 0x4f74, 0x90, 0xe2, 0x56, 0x84, 0xc9, 0xcd, 0x94, 0x9b);

typedef void (__stdcall *DatasteadRTSP_RawSampleCallback) (void *Sender, int StreamNumber, char *CodecName, __int64 SampleTime_Absolute, __int64 SampleTime_Relative, BYTE *Buffer, int BufferSize, char *InfoString); 
typedef void (__stdcall *DatasteadRTSP_VideoSampleCallback) (void *Sender, __int64 SampleTime_Absolute, __int64 SampleTime_Relative, BYTE *Buffer, int BufferSize, int VideoWidth, int VideoHeight, int BitCount, int Stride); 
typedef void (__stdcall *DatasteadRTSP_AudioSampleCallback) (void *Sender, __int64 SampleTime_Absolute, __int64 SampleTime_Relative, BYTE *Buffer, int BufferSize, int SamplesPerSec, int Channels, int BitsPerSample); 

MIDL_INTERFACE("799485B3-0DA1-4F74-90E2-5684C9CD949B")
IDatasteadRTSPSampleCallback : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRawSampleCallback(DatasteadRTSP_RawSampleCallback SampleCallback,void *Sender) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetVideoRGBSampleCallback(DatasteadRTSP_VideoSampleCallback SampleCallback,void *Sender) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetAudioPCMSampleCallback(DatasteadRTSP_AudioSampleCallback SampleCallback,void *Sender) = 0;
};

//_________________________________________________________________________________

#define RTSP_Action_OpenURL                             L"RTSP_Action_OpenURL"
#define RTSP_Action_OpenURLAsync                        L"RTSP_Action_OpenURLAsync"
#define RTSP_Action_RecordToNewFileNow                  L"RTSP_Action_RecordToNewFileNow"
#define RTSP_Action_CancelPendingConnection             L"RTSP_Action_CancelPendingConnection"
#define RTSP_Action_Pause_URL                           L"RTSP_Action_Pause_URL"
#define RTSP_Action_Resume_URL                          L"RTSP_Action_Resume_URL"
#define RTSP_Action_CaptureFrame                        L"RTSP_Action_CaptureFrame"

#define RTSP_Source_RecordingFileName_str               L"RTSP_Source_RecordingFileName_str" 
#define RTSP_Source_MaxAnalyzeDuration_int              L"RTSP_Source_MaxAnalyzeDuration_int"
#define RTSP_Source_AutoReconnect_bool                  L"RTSP_Source_AutoReconnect_bool"
#define RTSP_Source_NoTranscoding_bool                  L"RTSP_Source_NoTranscoding_bool"
#define RTSP_Source_DeviceLostTimeOut_int               L"RTSP_Source_DeviceLostTimeOut_int"
#define RTSP_Source_BufferDuration_int                  L"RTSP_Source_BufferDuration_int" 
#define RTSP_Source_LowDelay_int                        L"RTSP_Source_LowDelay_int" 
#define RTSP_Source_ConnectionTimeOut_int               L"RTSP_Source_ConnectionTimeOut_int"
#define RTSP_Source_RTSPTransport_int                   L"RTSP_Source_RTSPTransport_int"
#define RTSP_Source_Format_str                          L"RTSP_Source_Format_str"
#define RTSP_Source_AuthUser_str                        L"RTSP_Source_AuthUser_str" 
#define RTSP_Source_AuthPassword_str                    L"RTSP_Source_AuthPassword_str" 
#define RTSP_Source_StreamInfo_str                      L"RTSP_Source_StreamInfo_str"
#define RTSP_Source_StartTime_int                       L"RTSP_Source_StartTime_int"
#define RTSP_Source_IsURLConnected_bool                 L"RTSP_Source_IsURLConnected_bool"

#define RTSP_VideoStream_Enabled_bool                   L"RTSP_VideoStream_Enabled_bool" 
#define RTSP_VideoStream_Synchronized_bool              L"RTSP_VideoStream_Synchronized_bool" 
#define RTSP_VideoStream_Recorded_bool                  L"RTSP_VideoStream_Recorded_bool"
#define RTSP_VideoStream_Index_int                      L"RTSP_VideoStream_Index_int" 
#define RTSP_VideoStream_PinFormat_str                  L"RTSP_VideoStream_PinFormat_str" 
#define RTSP_VideoStream_Width_int                      L"RTSP_VideoStream_Width_int" 
#define RTSP_VideoStream_Height_int                     L"RTSP_VideoStream_Height_int" 
#define RTSP_VideoStream_FrameRateMax_double            L"RTSP_VideoStream_FrameRateMax_double"
#define RTSP_VideoStream_Filter_str                     L"RTSP_VideoStream_Filter_str" 

#define RTSP_AudioStream_Enabled_bool                   L"RTSP_AudioStream_Enabled_bool"
#define RTSP_AudioStream_Recorded_bool                  L"RTSP_AudioStream_Recorded_bool"
#define RTSP_AudioStream_Index_int                      L"RTSP_AudioStream_Index_int"
#define RTSP_AudioStream_Filter_str                     L"RTSP_AudioStream_Filter_str" 
#define RTSP_AudioStream_Volume_int                     L"RTSP_AudioStream_Volume_int"

#define RTSP_FrameCapture_Width_int                     L"RTSP_FrameCapture_Width_int"
#define RTSP_FrameCapture_Height_int                    L"RTSP_FrameCapture_Height_int"
#define RTSP_FrameCapture_Time_int                      L"RTSP_FrameCapture_Time_int"
#define RTSP_FrameCapture_FileName_str                  L"RTSP_FrameCapture_FileName_str"

#define RTSP_CurrentRecording_FileSizeKb_int            L"RTSP_CurrentRecording_FileSizeKb_int"
#define RTSP_CurrentRecording_VideoFrameCount_int       L"RTSP_CurrentRecording_VideoFrameCount_int"
#define RTSP_CurrentRecording_ClipDurationMs_int        L"RTSP_CurrentRecording_ClipDurationMs_int"
#define RTSP_CurrentRecording_FileName_str              L"RTSP_CurrentRecording_FileName_str"

#define RTSP_LastRecorded_FileSizeKb_int                L"RTSP_LastRecorded_FileSizeKb_int"
#define RTSP_LastRecorded_ClipDurationMs_int            L"RTSP_LastRecorded_ClipDurationMs_int"
#define RTSP_LastRecorded_VideoFrameCount_int           L"RTSP_LastRecorded_VideoFrameCount_int"
#define RTSP_LastRecorded_FileName_str                  L"RTSP_LastRecorded_FileName_str"

#define RTSP_Dest_URL_str                               L"RTSP_Dest_URL_str"
#define RTSP_Dest_Video_BitRate_int                     L"RTSP_Dest_Video_BitRate_int"
#define RTSP_Dest_Video_Quality_int                     L"RTSP_Dest_Video_Quality_int"
#define RTSP_Dest_Video_KeyFrameInterval_int            L"RTSP_Dest_Video_KeyFrameInterval_int"

#define RTSP_Filter_Version_int                         L"RTSP_Filter_Version_int"
#define RTSP_Filter_Build_int                           L"RTSP_Filter_Build_int"
#define RTSP_Filter_LicenseKey_str                      L"RTSP_Filter_LicenseKey_str"

