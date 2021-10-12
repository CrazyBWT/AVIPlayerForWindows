#ifndef _CAVI_H
#define _CAVI_H
#include "framework.h"
#include "vfw.h"
#include "dsound.h"

extern LPD3DXSPRITE			g_pSpirit;

#define  SAFE_DELETE(p)				{ if ( p ) { delete ( p );     ( p ) = NULL; } }
#define  SAFE_DELETEARR(p)			{ if ( p ) { delete [] p;      ( p ) = NULL; } }
#define  SAFE_RELEASE(p)			{ if ( p ) { ( p )->Release(); ( p ) = NULL; } }

class CAvi
{
public:
	 CAvi();
	~CAvi();

	VOID	Init();
	VOID	Release();

	BOOL	Create(LPDIRECT3DDEVICE9 lpDD, LPDIRECTSOUND lpDS, const CHAR* pszFile, BOOL bLoop = FALSE, BOOL bRevPlay = FALSE);
	BOOL	Start();
	VOID	Stop();
	VOID	NextFrame();
	VOID	ReadFrame(LONG lFrame);
	BOOL	GetSound();
	HRESULT DrawBuffer();

	BOOL	Draw(LPDIRECT3DTEXTURE9 lpddsDest);

	__inline VOID SetDestRect(RECT rcDst)
	{
		m_rcDst = rcDst;
	}
	__inline VOID SetDestRect(INT nLeft, INT nTop, INT nRight, INT nBottom)
	{
		SetRect(&m_rcDst, nLeft, nTop, nRight, nBottom);
	}

public:
	BOOL				 m_bPlaying;
	BOOL				 m_bRevPlay;
	BOOL				 m_bLoop;
	LONG				 m_lIndex;
	LONG				 m_lFrames;
	DWORD				 m_dwFps;
	INT					 m_nTimerID;
	INT					 m_nTimeTick;

	LPBYTE				 m_lpInput;
	LPBYTE				 m_lpOutput;
	LONG				 m_lLength;
	LONG				 m_lLinePitch;
	DWORD				 m_dwLoadPos;
	DWORD				 m_dwLoadSize;
	INT					 m_nSoundFramesAhead;

	RECT				 m_rcSrc;
	RECT				 m_rcDst;

	CRITICAL_SECTION	 m_AccessBuffer;
	HIC					 m_Decompressor;
	PAVIFILE			 m_pAviFile;

	PAVISTREAM			 m_pAviSound;
	AVISTREAMINFO		 m_stSoundInfo;
	WAVEFORMATEX*		 m_pSoundFormat;
	LPDIRECTSOUNDBUFFER  m_lpdsbAvi;

	PAVISTREAM			 m_pAviStream;
	AVISTREAMINFO		 m_stStreamInfo;
	LPBITMAPINFOHEADER	 m_pInputFormat;
	LPBITMAPV4HEADER	 m_pTargetFormat;

	LPDIRECT3DTEXTURE9 m_lpddsAvi;
	
};

#endif // _CAVI_H