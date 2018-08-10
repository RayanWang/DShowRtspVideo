#include <streams.h>
#include <tchar.h>
#include <dshow.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <atlbase.h>
#include <string.h>
#include <stdlib.h>


#include "live.h"
#include "SPropParameterSetParser.h"

#pragma  comment( lib, "live.lib" )
//#pragma  comment(lib "ws2_32.lib")

static void StreamClose(void *p_private);

BOOL DebugTrace(char *lpszFormat,...)
{
	static HWND hwnd = ::FindWindow(_T("Delta-DbgView"),	_T("DbgView"));
	if(!IsWindow(hwnd))
		hwnd = ::FindWindow(NULL, _T("DbgView"));
	if(hwnd)
	{
		static char szMsg[512];
		va_list argList;
		va_start(argList, lpszFormat);
		try
		{
			vsprintf_s(szMsg,lpszFormat, argList);
		}
		catch(...)
		{
			strcpy_s(szMsg, "DebugHelper:Invalid string format!");
		}
		va_end(argList);

		//OutputDebugStringA(szMsg);
		DWORD dwId = GetCurrentProcessId();


		int   nLen   =   strlen(szMsg)   +   1;   
		int   nwLen   =   MultiByteToWideChar(CP_ACP,   0,   szMsg,   nLen,   NULL,   0);   

		WCHAR   lpszFile[512] ={0} ;   
		MultiByteToWideChar(CP_ACP,   0,   szMsg,   nLen,   lpszFile,   nwLen);

		::SendMessage(hwnd,WM_SETTEXT,dwId,(LPARAM)lpszFile);
	}
	return TRUE;
}


inline FrameInfo* FrameNew(int frame_size = 4096)
{
	FrameInfo* frame = (FrameInfo*)malloc(sizeof(FrameInfo)+frame_size);
	if (frame == NULL)
		return(NULL);
	frame->pdata = (char *)frame + sizeof(FrameInfo);//new char[frame_size];	
	frame->frameHead.FrameLen = frame_size;
	return(frame);
}


static unsigned char* parseH264ConfigStr( char const* configStr,
										 unsigned int& configSize )
{
	char *dup, *psz;
	int i, i_records = 1;

	if( configSize )
		configSize = 0;

	if( configStr == NULL || *configStr == '\0' )
		return NULL;

	psz = dup = _strdup( configStr );

	/* Count the number of comma's */
	for( psz = dup; *psz != '\0'; ++psz )
	{
		if( *psz == ',')
		{
			++i_records;
			*psz = '\0';
		}
	}

	unsigned char *cfg = new unsigned char[5 * strlen(dup)];
	psz = dup;
	for( i = 0; i < i_records; i++ )
	{
		cfg[configSize++] = 0x00;
		cfg[configSize++] = 0x00;
		cfg[configSize++] = 0x00;
		cfg[configSize++] = 0x01;

		configSize += CStreamMedia::b64_decode( (char*)&cfg[configSize], psz );
		psz += strlen(psz)+1;
	}

	free( dup );
	return cfg;
}


void CStreamMedia::rtspClientSetTcpStream()
{
	m_bTCP_Stream = 1;
}


int CStreamMedia::rtspClientOpenStream(const char* filename, int flag)
{
	int verbosityLevel = 0;
	int tunnelOverHTTPPortNum = 0;
	int result;
	char* sdpDescription;
	MediaSubsessionIterator *iter   = NULL;
    MediaSubsession         *sub    = NULL;
	StreamTrack *tk;

	stream		= NULL;
	ms 			= NULL;
	scheduler 	= NULL;
	env  		= NULL;
	rtsp 		= NULL;	 
	m_nStream	= 0;
	m_nCurValidStream = 0;
	m_nNoStream	= 0;
	event		= 0;
	 
	if (m_state >= RTSP_STATE_OPENED)
	{
		err("rtspClientOpenStream already open \n");
		return 0;
	}

	if (strstr(filename, ".sdp"))
	{
		frameQueue.setLive();
	}
 
	scheduler =  BasicTaskScheduler::createNew();
	if (scheduler == NULL)
	{
		err("BasicTaskScheduler fail \n");
		goto fail;
	}

	env = BasicUsageEnvironment::createNew(*scheduler);
	if (env == NULL)
	{
		err("BasicUsageEnvironment fail \n");
		goto fail;
	}

    rtsp = RTSPClient::createNew(*env, verbosityLevel, AppName);
	if (rtsp == NULL)
	{		 
		err("create rtsp client fail \n"); 
		goto fail;
	}

	if (rtsp->sendOptionsCmd(filename) == NULL)
	{
		err("send optioncmd fail\n");
		goto fail;
	}
	 
	if (m_szUserName != NULL && m_szPassWord != NULL) 
	{
passwordDescribe:
		sdpDescription = rtsp->describeWithPassword(filename, m_szUserName, m_szPassWord);
  	}
	else 
	{
   		sdpDescription = rtsp->describeURL(filename);
	}
	if (sdpDescription == NULL)
	{
		err("rtspClient->describeStatus \n");	
		const char *psz_error = env->getResultMsg();
		const char *psz_tmp = strstr( psz_error, "RTSP" );
		if( psz_tmp )
			sscanf_s( psz_tmp, "RTSP/%*s%3u", &result);
		else
			result = 0;
		if (result == 401)
		{
			//get UserName & PassWord from interface;
			if (0)
			{
				goto passwordDescribe;
			}
			
		}

		goto fail;
	}
	ms = MediaSession::createNew(*env, sdpDescription);
 	delete[] sdpDescription;
	if (ms == NULL)
	{
		err("create MediaSession fail \n");		
		goto fail;
	}
	
	iter = new MediaSubsessionIterator(*ms);
    while( ( sub = iter->next() ) != NULL )
    {
        Boolean bInit;
       
		int i_buffer = 0;
		unsigned const thresh = 200000; /* RTP reorder threshold .2 second (default .1) */
       
        /* Value taken from mplayer */
        if( !strcmp( sub->mediumName(), "audio" ) )
            i_buffer = 100000;
        else if( !strcmp( sub->mediumName(), "video" ) )
            i_buffer = 2000000;
        else 
			continue;
       
        if( !strcmp( sub->codecName(), "X-ASF-PF" ) )
            bInit = sub->initiate( 4 ); /* Constant ? */
        else
            bInit = sub->initiate();

        if( !bInit )
        {
            err("RTP subsession '%s/%s' failed (%s)",
				sub->mediumName(), sub->codecName(), env->getResultMsg() );
        }
        else
        {
            if( sub->rtpSource() != NULL )
            {
                int fd = sub->rtpSource()->RTPgs()->socketNum();

                /* Increase the buffer size */
                if( i_buffer > 0 )
                    increaseReceiveBufferTo(*env, fd, i_buffer );

                /* Increase the RTP reorder timebuffer just a bit */
                sub->rtpSource()->setPacketReorderingThresholdTime(thresh);
            }
           
            /* Issue the SETUP */
            if(rtsp)
            {
                if( !rtsp->setupMediaSubsession(*sub, false, m_bTCP_Stream))
                {
                    /* if we get an unsupported transport error, toggle TCP use and try again */
                    if( !strstr(env->getResultMsg(), "461 Unsupported Transport")
                     || !rtsp->setupMediaSubsession( *sub, false, m_bTCP_Stream ? false : true))
                    {
                        err("SETUP of'%s/%s' failed %s", sub->mediumName(), sub->codecName(), env->getResultMsg() );
                        continue;
                    }
                }
            }

            /* Check if we will receive data from this subsession for this track */
            if( sub->readSource() == NULL )
				continue;
           
            tk = new StreamTrack;
            if( !tk )
            {
                delete iter;
                return (-1);
            }
			tk->pstreammedia	= this;
            tk->sub				= sub;			
			tk->waiting			= 0;
			ZeroMemory(&tk->mediainfo, sizeof(MediaInfo));
			tk->mediainfo.b_packetized = 1;
			strncpy_s(tk->mediainfo.codecName, sub->codecName(), sizeof(tk->mediainfo.codecName));
			tk->mediainfo.duration = (int)(sub->playEndTime() - sub->playStartTime());
           
            /* Value taken from mplayer */
            if( !strcmp( sub->mediumName(), "audio" ) )
            {
            	tk->i_buffer = 4096;
				tk->p_buffer = new char[tk->i_buffer];
            	tk->mediainfo.codecType = CODEC_TYPE_AUDIO;
				tk->mediainfo.audio.channels = sub->numChannels();
				tk->mediainfo.audio.sampleRate = sub->rtpTimestampFrequency();
                tk->mediainfo.i_format = FOURCC('u', 'n', 'd', 'f');
				m_nCurValidStream |= (0x1<<CODEC_TYPE_AUDIO);

                if( !strcmp( sub->codecName(), "MPA" ) ||
                    !strcmp( sub->codecName(), "MPA-ROBUST" ) ||
                    !strcmp( sub->codecName(), "X-MP3-DRAFT-00" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 'm', 'p', 'g', 'a' );
                }
                else if( !strcmp( sub->codecName(), "AC3" ) )
                {
                     tk->mediainfo.i_format = FOURCC( 'a', '5', '2', ' ' );
                }
                else if( !strcmp( sub->codecName(), "L16" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 't', 'w', 'o', 's' );
                     tk->mediainfo.audio.i_bitspersample = 16;
                }
                else if( !strcmp( sub->codecName(), "L8" ) )
                {
                     tk->mediainfo.i_format = FOURCC( 'a', 'r', 'a', 'w' );
                     tk->mediainfo.audio.i_bitspersample = 8;
                }
                else if( !strcmp( sub->codecName(), "PCMU" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 'u', 'l', 'a', 'w' );
                }
                else if( !strcmp( sub->codecName(), "PCMA" ) )
                {
					tk->mediainfo.i_format = FOURCC( 'a', 'l', 'a', 'w' );
                }
                else if( !strncmp( sub->codecName(), "G726", 4 ) )
                {
                    tk->mediainfo.i_format = FOURCC( 'g', '7', '2', '6' );
                    tk->mediainfo.audio.sampleRate = 8000;
                    tk->mediainfo.audio.channels   = 1;
					if( !strcmp( sub->codecName()+5, "40" ) )
						tk->mediainfo.audio.sampleRate = 40000;
					else if( !strcmp( sub->codecName()+5, "32" ) )
						tk->mediainfo.audio.sampleRate = 32000;
					else if( !strcmp( sub->codecName()+5, "24" ) )
						tk->mediainfo.audio.sampleRate = 24000;
					else if( !strcmp( sub->codecName()+5, "16" ) )
						tk->mediainfo.audio.sampleRate = 16000;
                }
                else if( !strcmp( sub->codecName(), "AMR" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 's', 'a', 'm', 'r' );
                }
                else if( !strcmp( sub->codecName(), "AMR-WB" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 's', 'a', 'w', 'b' );
                }
                else if( !strcmp( sub->codecName(), "MP4A-LATM" ) )
                {
                    unsigned int i_extra;
                    char		*p_extra;

                    tk->mediainfo.i_format = FOURCC( 'm', 'p', '4', 'a' );

                    if( ( p_extra = (char *)parseStreamMuxConfigStr( sub->fmtp_config(), i_extra ) ) )
                    {
                        tk->mediainfo.extra_size = i_extra;
                        tk->mediainfo.extra  = new char[i_extra];
                        memcpy(tk->mediainfo.extra, p_extra, i_extra );
                        delete [] p_extra;
                    }
                    /* Because the "faad" decoder does not handle the LATM data length field
                       at the start of each returned LATM frame, tell the RTP source to omit it. */
                    ((MPEG4LATMAudioRTPSource*)sub->rtpSource())->omitLATMDataLengthField();
                }
                else if( !strcmp( sub->codecName(), "MPEG4-GENERIC" ) )
                {
                    unsigned int i_extra;
                    char		*p_extra;

                    tk->mediainfo.i_format = FOURCC( 'm', 'p', '4', 'a' );

                    if( ( p_extra = (char *)parseGeneralConfigStr( sub->fmtp_config(), i_extra ) ) )
                    {
                        tk->mediainfo.extra_size = i_extra;
                        tk->mediainfo.extra  = new char[i_extra];
                        memcpy(tk->mediainfo.extra, p_extra, i_extra );
                        delete [] p_extra;
                    }
                }
            }
            else if( !strcmp( sub->mediumName(), "video" ) )
            {
            	tk->i_buffer = 65536;
				tk->p_buffer = new char[tk->i_buffer];
            	tk->mediainfo.codecType = CODEC_TYPE_VIDEO;
            	tk->mediainfo.i_format	= FOURCC('u','n','d','f');
				tk->mediainfo.video.fps = sub->videoFPS();

				tk->mediainfo.video.height	= sub->videoHeight();
				tk->mediainfo.video.width	= sub->videoWidth();
				
				m_nCurValidStream |= (0x1<<CODEC_TYPE_VIDEO);
                if( !strcmp( sub->codecName(), "MPV" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 'm', 'p', 'g', 'v' );
                }
                else if( !strcmp( sub->codecName(), "H263" ) ||
                         !strcmp( sub->codecName(), "H263-1998" ) ||
                         !strcmp( sub->codecName(), "H263-2000" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 'H', '2', '6', '3' );
                }
                else if( !strcmp( sub->codecName(), "H261" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 'H', '2', '6', '1' );
                }
                else if( !strcmp( sub->codecName(), "H264" ) )
                {
                    unsigned int i_extra = 0;
                    char		*p_extra = NULL;

                    tk->mediainfo.i_format = FOURCC( 'h', '2', '6', '4' );
					tk->mediainfo.b_packetized  = false;

                    if((p_extra = (char *)parseH264ConfigStr( sub->fmtp_spropparametersets(), i_extra ) ) )
                    {
                        tk->mediainfo.extra_size = i_extra;
                        tk->mediainfo.extra  = new char[i_extra];
                        memcpy(tk->mediainfo.extra, p_extra, i_extra );

                        delete [] p_extra;

						CSPropParameterSetParser oSPropParser(sub->fmtp_spropparametersets());
						tk->mediainfo.video.height	= oSPropParser.GetHeight();
						tk->mediainfo.video.width	= oSPropParser.GetWidth();
                    }
                }
                else if( !strcmp( sub->codecName(), "JPEG" ) )
                {
                    tk->mediainfo.i_format = FOURCC( 'M', 'J', 'P', 'G' );
                }
                else if( !strcmp( sub->codecName(), "MP4V-ES" ) )
                {
                    unsigned int i_extra;
                    char		*p_extra;

                    tk->mediainfo.i_format = FOURCC( 'm', 'p', '4', 'v' );

                    if( ( p_extra = (char *)parseGeneralConfigStr( sub->fmtp_config(), i_extra ) ) )
                    {
                        tk->mediainfo.extra_size = i_extra;
                        tk->mediainfo.extra  = new char[i_extra];
                        memcpy(tk->mediainfo.extra, p_extra, i_extra );
                        delete [] p_extra;
                    }
                }
                else if( !strcmp( sub->codecName(), "MP2P" ) ||
                         !strcmp( sub->codecName(), "MP1S" ) )
                {
                    ;
                }
                else if( !strcmp( sub->codecName(), "X-ASF-PF" ) )
                {
                   ;
                }
            }
        
            if( sub->rtcpInstance() != NULL )
            {
                ;//sub->rtcpInstance()->setByeHandler( StreamClose, tk );
            }

            stream = (StreamTrack**)realloc( stream, sizeof( StreamTrack ) * (m_nStream+1));
            stream[m_nStream++] = tk;
        }
    } //while find track;
	
	delete iter;
	if (m_nStream <= 0)
	{
		err("don't find stream \n");
		goto fail;
	}
	m_state = RTSP_STATE_OPENED;

	return(0);

fail:
	m_state = RTSP_STATE_IDLE;
	if (stream)
	{			
		free(stream);
		stream = NULL;
	}
	/*if (rtsp && ms)
	{
		rtsp->teardownMediaSession(*ms);
	}*/
	
	//if (ms)
	//{
	//	Medium::close(ms);
	//}
	//
   /* if(rtsp) 
		RTSPClient::close(rtsp);
    if(env) 
	{
		env->reclaim();
		env = NULL;
	}*/
	if (ms)
	{
		Medium::close(ms);
		ms = NULL;
	}
	if(rtsp) 
	{
		RTSPClient::close(rtsp);			
		rtsp = NULL;
	}
	if(env)
	{
		env->reclaim();
		env = NULL;
	}
	if (scheduler)
	{
		delete scheduler;
		scheduler = NULL;
	}
	
	return(-1);
}

void StreamRead( void *p_private, unsigned int i_size,
				unsigned int i_truncated_bytes, struct timeval pts,
				unsigned int duration );
//void StreamClose(void *p_private);

int CStreamMedia::rtspClientPlayStream()
{
	if (m_state == RTSP_STATE_OPENED)
	{
		event			= 0;
		m_hRecvEvent		= CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hRecvDataThread	= CreateThread(NULL, 0, rtspRecvDataThread, this, 0, NULL);
		m_hDataThreadReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	//	SetThreadPriority(m_hRecvDataThread, THREAD_PRIORITY_BELOW_NORMAL);
		//Sleep(5);
		if (WaitForSingleObject(m_hDataThreadReady, 500) == WAIT_TIMEOUT)
		{
			err("rtspClientPlayStream timeout \n");
			return(-1);
		}
		err("rtspClientPlayStream recive thread ready \n");
		if (!rtsp->playMediaSession(*ms, 0.0, -1, 1 ))
		{
			err("playMediaSession fail \n");
			return(-1);
		}
	}
	else if (m_state == RTSP_STATE_PAUSED)
	{
		if (!rtsp->playMediaSession(*ms, -1))
		{
			err("playMediaSession fail \n");
			return(-1);
		}
	}
	else
	{
		return(-1);
	}
#if 0
	if (m_state == RTSP_STATE_OPENED)
	{
		SetEvent(m_hRecvEvent);
	}
#endif
	m_state  = RTSP_STATE_PLAYING;
	
#if 0
	{
		StreamTrack   *tk;
		for( int i = 0; i < m_nStream; i++ )
		{
			tk = stream[i];

			if( tk->waiting == 0 )
			{
				tk->waiting = 1;
				tk->sub->readSource()->getNextFrame((unsigned char *) tk->p_buffer, tk->i_buffer,
					StreamRead, tk, StreamClose, this);
			}
		}
		/* Create a task that will be called if we wait more than 300ms */
		//    task = rtspClient->scheduler->scheduleDelayedTask( 300000, TaskInterrupt, p_demux );

		/* Do the read */
		scheduler->doEventLoop();
		m_bRecvThreadFlag = FALSE;
	}
#endif
	return(0);
}

int CStreamMedia::rtspClientPauseStream()
{
	if (m_state  != RTSP_STATE_PLAYING)
	{
		return(-1);
	}
	m_state  = RTSP_STATE_PAUSED;
	if (!rtsp->pauseMediaSession(*ms))
	{
		char *errmsg;

		errmsg = (char *)env->getResultMsg();
		return(-1);
	}

	return(0);
}

int CStreamMedia::rtspClientStopStream()
{
	rtspClientCloseStream();
	
//	CloseHandle(m_hRecvEvent);
//	CloseHandle(m_hRecvDataThread);
	/*if (rtsp && ms)
	{
		rtsp->teardownMediaSession(*ms);
	}*/
/*		if (stream)
	{
		free(stream);
		stream = NULL;
	}
		
 
    if(rtsp) 
		RTSPClient::close(rtsp);
	rtsp = NULL;
	ms = NULL;

  /*  if(env) 
		env->reclaim();*/

	m_state  = RTSP_STATE_IDLE;
	return(0);
}

int CStreamMedia::rtspClientSeekStream(long timestamp, int flag)
{
//todo 
	if (m_state == RTSP_STATE_IDLE)
	{
		return(-1);
	}
	frameQueue.reset();
	if (!rtsp->playMediaSession(*ms, timestamp/1.0 , -1, 1 ))
		return(-1);
	
	return(0);
}

int CStreamMedia::rtspClinetGetMediaInfo(enum CodecType codectype, MediaInfo& mediainfo)
{
	StreamTrack *tr;
	for (int i = 0; i < m_nStream; i++)
	{
		tr = (StreamTrack*)stream[i];
		if (tr->mediainfo.codecType == codectype)
		{
			mediainfo = tr->mediainfo;
			return(0);
		}
	}

	return(-1);
}

int CStreamMedia::rtspClientCloseStream(void)
{
	StreamTrack *tr;

	if (m_state == RTSP_STATE_IDLE)
	{
		return(0);
	}
	frameQueue.empty();	
	event  = 0xff;

	if (rtsp && ms)
	{
		err("begin teardownMediaSession \n");
		rtsp->teardownMediaSession(*ms);
		err("end teardownMediaSession \n");
	}
	
	m_state = RTSP_STATE_IDLE;
	m_nNoStream = 1;
	err("begin WaitForSingleObject \n");
	if (WaitForSingleObject(m_hRecvEvent, 500) == WAIT_TIMEOUT)
	{
		err("rtspClientCloseStream WAIT_TIMEOUT\n");
		DWORD dwExitCode = 0;
		
		TerminateThread(m_hRecvDataThread, dwExitCode);
	}
	if (stream)
	{			
		for (int i = 0; i < m_nStream; i++)
		{
			tr = (StreamTrack*)stream[i];
			if (tr)
			{
				delete tr;
			}
		}

		free(stream);
		stream = NULL;
	}

	if (ms)
	{
		Medium::close(ms);
		ms = NULL;
	}
	if(rtsp) 
	{
		RTSPClient::close(rtsp);			
		rtsp = NULL;
	}
	if(env)
	{
		env->reclaim();
		env = NULL;
	}
	if (scheduler)
	{
		delete scheduler;
		scheduler = NULL;
	}
	if (m_hDataThreadReady != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hDataThreadReady);
	}
	if (m_hRecvEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hRecvEvent);
	}
	if (m_hRecvDataThread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hRecvDataThread);
	}
	
	err("rtspClientCloseStream \n");
	/*	SetEvent(m_hRecvEvent);*/

	return(0);
}

int CStreamMedia::rtspClientReadFrame(FrameInfo*& frame)
{
	frame = NULL;

	frame  = frameQueue.get();
	if (frame)
	{
		err("!!rtspClientReadFrame len = %d  count = %d\n", frame->frameHead.FrameLen, frameQueue.count);		
		return(frame->frameHead.FrameLen);
	}
	
	err("!!!! rtspClientReadFrame fail \n");
	return(-1);
}

void CStreamMedia::SetAccount(WCHAR* pwszAccount)
{
	char szAccount[MAX_PATH] = {0};		
	WideCharToMultiByte (CP_OEMCP, NULL, pwszAccount, -1, (LPSTR)szAccount, MAX_PATH, NULL, FALSE);
	m_szUserName = new char[MAX_PATH];
	strcpy_s(m_szUserName, MAX_PATH, szAccount);
}

void CStreamMedia::SetPassword(WCHAR* pwszPassword)
{
	char szPassword[MAX_PATH] = {0};		
	WideCharToMultiByte (CP_OEMCP, NULL, pwszPassword, -1, (LPSTR)szPassword, MAX_PATH, NULL, FALSE);
	m_szPassWord = new char[MAX_PATH];
	strcpy_s(m_szPassWord, MAX_PATH, szPassword);
}


/*char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";*/
int CStreamMedia::b64_decode( char *dest, char *src )
{
	const char *dest_start = dest;
	int  i_level;
	int  last = 0;
	int  b64[256] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
		52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
		15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
		-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
		41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
	};

	for( i_level = 0; *src != '\0'; src++ )
	{
		int  c;

		c = b64[(unsigned int)*src];
		if( c == -1 )
		{
			continue;
		}

		switch( i_level )
		{
		case 0:
			i_level++;
			break;
		case 1:
			*dest++ = ( last << 2 ) | ( ( c >> 4)&0x03 );
			i_level++;
			break;
		case 2:
			*dest++ = ( ( last << 4 )&0xf0 ) | ( ( c >> 2 )&0x0f );
			i_level++;
			break;
		case 3:
			*dest++ = ( ( last &0x03 ) << 6 ) | c;
			i_level = 0;
		}
		last = c;
	}

	*dest = '\0';

	return dest - dest_start;
}

static void StreamRead( void *p_private, unsigned int i_size,
                        unsigned int i_truncated_bytes, struct timeval pts,
                        unsigned int duration )
{
    StreamTrack   *tk = (StreamTrack*)p_private;
	CStreamMedia  *rtspClient = (CStreamMedia*)(tk->pstreammedia);
    FrameInfo	  *p_block;
    unsigned long i_pts = pts.tv_sec*1000 + pts.tv_usec/1000;
	err("recv %s data len = %d i_truncated_bytes = %d i_pts = %ud sec = %d usec = %d duration = %d \n", tk->mediainfo.codecType == CODEC_TYPE_VIDEO?"video":"audio", i_size, i_truncated_bytes, i_pts,pts.tv_sec, pts.tv_usec, duration);
	//err("StreamRead %s recv data len = %d count = %d\r", tk->mediainfo.codecType==CODEC_TYPE_VIDEO?"video":"audio",i_size, count++);

    if( i_size > tk->i_buffer )
    {
        err("buffer overflow" );
    }
	
    if( tk->mediainfo.i_format == FOURCC('s','a','m','r') ||
        tk->mediainfo.i_format == FOURCC('s','a','w','b') )
    {
        AMRAudioSource *amrSource = (AMRAudioSource*)tk->sub->readSource();

        p_block = FrameNew(i_size + 1);
        p_block->pdata[0] = amrSource->lastFrameHeader();
        memcpy( p_block->pdata+ 1, tk->p_buffer, i_size );
    }
    else if( tk->mediainfo.i_format == FOURCC('H','2','6','1') )
    {
        H261VideoRTPSource *h261Source = (H261VideoRTPSource*)tk->sub->rtpSource();
        uint32_t header = h261Source->lastSpecialHeader();
        p_block = FrameNew(i_size + 4);
        memcpy( p_block->pdata, &header, 4 );
        memcpy( p_block->pdata + 4, tk->p_buffer, i_size );
    }
    else if( tk->mediainfo.i_format == FOURCC('h','2','6','4') )
    {
        if( (tk->p_buffer[0] & 0x1f) >= 24 )
            err( "unsupported NAL type for H264" );

        /* Normal NAL type */
		if(rtspClient->m_bIsH264PacketHeader)
		{
			rtspClient->m_bIsH264PacketHeader = FALSE;
			int extra_size = tk->mediainfo.extra_size;
			p_block = FrameNew(i_size + extra_size + 4);
			memcpy( p_block->pdata, tk->mediainfo.extra, extra_size );
			p_block->pdata[extra_size] = 0x00;
			p_block->pdata[extra_size+1] = 0x00;
			p_block->pdata[extra_size+2] = 0x00;
			p_block->pdata[extra_size+3] = 0x01;
			memcpy( &p_block->pdata[extra_size+4], tk->p_buffer, i_size );
		}
		else
		{
			p_block = FrameNew(i_size + 4 );
			p_block->pdata[0] = 0x00;
			p_block->pdata[1] = 0x00;
			p_block->pdata[2] = 0x00;
			p_block->pdata[3] = 0x01;
			memcpy( &p_block->pdata[4], tk->p_buffer, i_size );
		}
    }
    else
    {
        p_block = FrameNew( i_size );
        memcpy( p_block->pdata, tk->p_buffer, i_size );
    }
	
	p_block->frameHead.FrameType = (long)(tk->mediainfo.codecType);
    p_block->frameHead.TimeStamp = i_pts;
	rtspClient->frameQueue.put(p_block);

	//static FILE *datafp = NULL;
	//int ret;

	//if (datafp == NULL)
	//{
	//	datafp = fopen("D:\\remotedata1.raw", "wb");
	//	if (datafp == NULL)
	//	{
	//		err("fopen file fail \n");
	//	}
	//}
	//if (datafp != NULL && tk->mediainfo.codecType == CODEC_TYPE_AUDIO)
	//{
	//	static char head1[7];
	//	int temp = i_size + 7;
	//	head1[0] = (char)0xff;
	//	head1[1] = (char)0xf9;
	////	int sr_index = rtp_aac_get_sr_index(aac_param_ptr->sample_rate);
	//	int sr_index = 4;
	//	head1[2] = (0x01<<6)|(sr_index<<2)|0x00;
	//	head1[3] = (char)0x80;
	//	head1[4] = (temp>>3)&0xff;
	//	head1[5] = ((temp&0x07)<<5|0x1f);
	//	head1[6] = (char)0xfc;

	//	//fwrite(head1,1,sizeof(head1),datafp);
	//	ret = fwrite(tk->p_buffer, 1, i_size, datafp);
	//	if (ret <= 0)
	//	{
	//		err("fwrite data fail \n");
	//	}
	//}

	tk->sub->readSource()->getNextFrame((unsigned char *) tk->p_buffer, tk->i_buffer, 
		StreamRead, tk, StreamClose, tk);
}

static void StreamClose(void *p_private)
{
	CStreamMedia* rtspClient = (CStreamMedia*)p_private;
	rtspClient->m_state = RTSP_STATE_OPENED;
}

static void TaskInterrupt( void *p_private )
{
	char *event = (char*)p_private;

	/* Avoid lock */
	*event = 0xff;
	err("TaskInterrupt");
}

DWORD WINAPI rtspRecvDataThread( LPVOID lpParam )
{
	CStreamMedia *rtspClient = (CStreamMedia*)lpParam;
	StreamTrack  *tk;
	char*		 event = &(rtspClient->event);

	//while(rtspClient->m_bRecvThreadFlag)
	{
		//WaitForSingleObject(rtspClient->m_hRecvEvent, INFINITE);
		err("rtspRecvDataThread begin  \n");

		*event = 0;
		for( int i = 0; i < rtspClient->m_nStream; i++ )
		{
			tk = rtspClient->stream[i];

			if(tk)
			{     
				tk->sub->readSource()->getNextFrame((unsigned char *) tk->p_buffer, tk->i_buffer, 
					StreamRead, tk, StreamClose, tk);
			}
		}
		
		SetEvent(rtspClient->m_hDataThreadReady);		
		rtspClient->scheduler->doEventLoop(event);//rtspClient->event);
		for( int i = 0; i < rtspClient->m_nStream; i++ )
		{
			if (rtspClient->stream)
			{
				tk = rtspClient->stream[i];
				{     
					tk->sub->readSource()->stopGettingFrames();
				}
			}
		}
		
		rtspClient->m_bRecvThreadFlag = FALSE;
		err("rtspRecvDataThread  data stop \n");
	}
	//if (rtspClient->m_nNoStream == 0)
	//{
	//	WaitForSingleObject(rtspClient->m_hRecvEvent, 60*1000);
	//}
	rtspClient->frameQueue.empty();
	rtspClient->m_nNoStream = 1;
	/*
	if (rtspClient->stream)
	{
		for (int i = 0; i < rtspClient->m_nStream; i++)
		{
			tk = (StreamTrack*)rtspClient->stream[i];
			if (tk->mediainfo.extra_size > 0)
			{
				tk->mediainfo.extra_size = 0;
				delete[] tk->mediainfo.extra;
				tk->mediainfo.extra = NULL;
			}
			delete tk;
		}
		free(rtspClient->stream);
		rtspClient->stream =NULL;
	}
	if (rtspClient->ms)
	{
		Medium::close(rtspClient->ms);
		rtspClient->ms = NULL;
	}
	if(rtspClient->rtsp) 
	{
		RTSPClient::close(rtspClient->rtsp);
		rtspClient->rtsp = NULL;
	}
	
	if(rtspClient->env) 
	{
		rtspClient->env->reclaim();
		rtspClient->env = NULL;
	}
	if (rtspClient->scheduler)
	{
		delete rtspClient->scheduler;
		rtspClient->scheduler = NULL;
	}*/
	SetEvent(rtspClient->m_hRecvEvent);
	
	err("rtspRecvDataThread end \n");

	return(0);
}

