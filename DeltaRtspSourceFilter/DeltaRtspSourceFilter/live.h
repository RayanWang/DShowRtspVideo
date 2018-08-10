#ifndef __LIVE_H__
#define __LIVE_H__

#include "UsageEnvironment.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"


#define AppName "COSHIPMPLAYER"


typedef unsigned char 	uint8_t;
typedef unsigned short 	uint16_t;
typedef unsigned int   	uint32_t;
typedef unsigned int  	fourcc_t;
#define FOURCC( a, b, c, d ) ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )

enum CodecType 
{
    CODEC_TYPE_UNKNOWN = -1,
    CODEC_TYPE_VIDEO,
    CODEC_TYPE_AUDIO,
    CODEC_TYPE_DATA,
    CODEC_TYPE_SUBTITLE,
    CODEC_TYPE_ATTACHMENT,
    CODEC_TYPE_NB
};

enum RTSPClientState 
{
    RTSP_STATE_IDLE,    /**< not initialized */
	RTSP_STATE_OPENED,
    RTSP_STATE_PLAYING, /**< initialized and receiving data */
    RTSP_STATE_PAUSED,  /**< initialized, but not receiving data */
};

typedef struct __FrameHeader
{
	long TimeStamp;
	long FrameType;
	long FrameLen;
} FrameHeader;


typedef struct __FrameInfo
{
    FrameHeader frameHead;
	char* pdata;
} FrameInfo;

typedef struct __MediaQueue
{
	FrameInfo *frame;
	struct __MediaQueue *next;
} MediaQueue;

typedef struct __VideoFormat
{
	int	width;
	int	height;
	int	fps;
	int bitrate;
} VideoFormat;

typedef struct __AudioFormat
{
	uint32_t	sampleRate;
	uint32_t	channels;
	uint32_t	i_bytes_per_frame;
	uint32_t	i_frame_length;
	uint32_t	i_bitspersample;
} AudioFormat;

typedef struct __MediaInfo
{
	enum CodecType codecType;
	fourcc_t  i_format;
	char codecName[50];
	AudioFormat audio;
	VideoFormat video;
	int	duration;
	int b_packetized;
	char*	extra;
	int     extra_size;	
} MediaInfo;

class CMediaQueue
{
#define  MAX_FRAME_COUNT 400
	MediaQueue *head, *tail;
	MediaQueue *writepos, *readpos;
	MediaQueue *ptr, *pstatic;
#if 1
	int isLive;
#endif

	HANDLE		m_hFrameListLock;
	HANDLE  	m_hRecvEvent;

public:
	int  count;

	CMediaQueue()
	{
		head = tail = NULL;
		ptr = pstatic = (MediaQueue *)malloc(sizeof(MediaQueue)*MAX_FRAME_COUNT);
		ZeroMemory(pstatic, sizeof(MediaQueue)*MAX_FRAME_COUNT);
		head = ptr;
		readpos = writepos = head;
		for (int i = 1; i < MAX_FRAME_COUNT; i++)
		{
			ptr->next = pstatic+i;
			ptr = ptr->next;
		}
		tail = ptr;
		tail->next =  head;
		ptr = head;
		count = 0;
		isLive = 0;
		m_hFrameListLock = CreateMutex(NULL,FALSE,NULL);
		m_hRecvEvent     = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	~CMediaQueue()
	{
		CloseHandle(m_hRecvEvent);
		CloseHandle(m_hFrameListLock);
		
		for(int i = 0; i < MAX_FRAME_COUNT; i++)
		{	 
			ptr = head;
			if(ptr)
			{					
				if(ptr->frame)
				{					
					free(ptr->frame);
					ptr->frame =  NULL;     		
				}
				ptr = ptr->next;
				
			}
		}
		free(pstatic);
	}

	void put(FrameInfo* frame)
	{	  
		if(ptr == NULL)
			return; 
		WaitForSingleObject(m_hFrameListLock,INFINITE);
		if (count >= MAX_FRAME_COUNT)
		{
			count = MAX_FRAME_COUNT-1;
			if (readpos->frame)
			{
				free(readpos->frame);
				readpos->frame = NULL;
			}
			readpos = readpos->next;
		}
		writepos->frame = frame;
		writepos =  writepos->next;
		count++;

		if (count <=1)
		{
			SetEvent(m_hRecvEvent);
		}
		ReleaseMutex(m_hFrameListLock);
	}

	FrameInfo* get()
	{
		FrameInfo* frame = NULL;
		
		if (count < 1)
		{
#if 1
			if (isLive == 0)
			{
				WaitForSingleObject(m_hRecvEvent, 8000);	
			}
			else
			{
				WaitForSingleObject(m_hRecvEvent, 60000);	
			}
				
#else
			WaitForSingleObject(m_hRecvEvent, 500);	
#endif
		}
		ResetEvent(m_hRecvEvent);
		
		WaitForSingleObject(m_hFrameListLock,INFINITE);
		if(count > 0)
		{			 
			frame = readpos->frame;
			readpos->frame =  NULL;
			readpos = readpos->next;  	
			count--;			
		}
		ReleaseMutex(m_hFrameListLock); 
		return(frame);
	}

	int size()
	{		
		return(count);
	}

	int isempty()
	{		
		return(count<=0?1:0);
	}

	int empty()
	{		
		SetEvent(m_hRecvEvent);
		return(count=0);
	}

#if 1
	void setLive()
	{
		isLive = 1;
	}

	void clearLive()
	{
		isLive = 0;
	}

	int getLive()
	{
		return(isLive);
	}
#endif

	void reset()
	{
		WaitForSingleObject(m_hFrameListLock,INFINITE);
		ptr = readpos;
		while (ptr->frame)
		{
			free(ptr->frame);
			ptr->frame = NULL;
			ptr = ptr->next;
		}		
		writepos = readpos;
		count  = 0;
		ReleaseMutex(m_hFrameListLock); 
	}
};

class CStreamMedia;

typedef struct __StreamTrack
{
    CStreamMedia		*pstreammedia;
	int 				waiting;
	MediaInfo   		mediainfo;
	MediaSubsession		*sub;
	//CMediaQueue		frameQueue;
	char				*p_buffer;
	unsigned int		i_buffer;		
} StreamTrack;

DWORD WINAPI rtspRecvDataThread( LPVOID lpParam );

class CStreamMedia
{
	BOOLEAN			 m_bRecvThreadFlag;

public:
    MediaSession     *ms;
    TaskScheduler    *scheduler;
    UsageEnvironment *env ;
    RTSPClient       *rtsp;	

	int				 m_nStream;
    StreamTrack      **stream;
	int				 m_nCurValidStream;
	CCritSec		 m_valid_stream_lock;
	int				 m_bTCP_Stream;

    //configure
	CMediaQueue				frameQueue;
	enum RTSPClientState	m_state;
	char					event;	
	HANDLE					m_hFrameListLock;
	HANDLE					m_hRecvDataThread;
	HANDLE					m_hDataThreadReady;
	HANDLE					m_hRecvEvent;
	int						m_nNoStream;

	BOOL					m_bIsH264PacketHeader;

private:
	char*					m_szUserName;
	char*					m_szPassWord;

public:
	friend DWORD WINAPI rtspRecvDataThread( LPVOID lpParam );
	
	CStreamMedia(/*CBaseFilter *baseFilter*/)
	{
		stream			= NULL;
		ms 				= NULL;
		scheduler 		= NULL;
		env  			= NULL;
		rtsp 			= NULL;
		m_szUserName 	= NULL;
		m_szPassWord 	= NULL;
		m_bTCP_Stream	= 1; // normal tcp
		m_nStream		= 0;
		m_nNoStream		= 0;
		event			= 0;
		m_state			= RTSP_STATE_IDLE;
		m_bRecvThreadFlag		= TRUE;
		m_bIsH264PacketHeader	= TRUE;
	}

	~CStreamMedia()
	{	
		StreamTrack *tr;
		m_state = RTSP_STATE_IDLE;			
		m_bRecvThreadFlag = FALSE;		
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

		if(m_szUserName)
		{
			delete [] m_szUserName;
			m_szUserName 	= NULL;
		}

		if (m_szPassWord)
		{
			delete [] m_szPassWord;
			m_szPassWord 	= NULL;
		}
	}
	
	int rtspClientm_nNoStream(void){ return(m_nNoStream); }
    void rtspClientSetTcpStream(void);
	int rtspClientOpenStream(const char* filename, int flag);
	int rtspClientPlayStream(void);
	int rtspClientPauseStream(void);
	int rtspClientStopStream(void);
	int rtspClientSeekStream(long timestamp, int flag);
	int rtspClientReadFrame(FrameInfo*& frame);
	int rtspClinetGetMediaInfo(enum CodecType codectype, MediaInfo& mediainfo);
	int rtspClientCloseStream(void);

	void SetAccount(WCHAR* pwszAccount);
	void SetPassword(WCHAR* pwszPassword);

	static int b64_decode( char *dest, char *src );
};

BOOL DebugTrace(char *lpszFormat,...);
#ifndef err
#define err DebugTrace

#endif

#endif

