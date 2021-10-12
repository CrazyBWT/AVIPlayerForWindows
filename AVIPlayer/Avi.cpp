#include "Avi.h"

const GUID CLSID_AVIFile = { 0x00020000, 0, 0, {0xC0, 0,  0,  0,  0,  0,  0,  0X46} };


CAvi::CAvi()
{
	Init();
	InitializeCriticalSection(&m_AccessBuffer);
}


CAvi::~CAvi(void)
{
	Release();
	DeleteCriticalSection(&m_AccessBuffer);
}


VOID CAvi::Init()
{
	m_bPlaying		= FALSE;
	m_bRevPlay		= FALSE;
	m_bLoop			= FALSE;
	m_dwFps			= 0;
	m_nTimeTick		= 0;
	m_lLength		= 0;
	m_lLinePitch	= 0;

	m_pAviFile		= NULL;
	m_nTimerID		= 0;

	m_pAviStream	= NULL;
	m_pInputFormat	= NULL;
	m_pTargetFormat = NULL;
	m_lpddsAvi		= NULL;
	m_lIndex		= 0;
	m_lFrames		= 0;
	m_Decompressor	= 0;
	m_lpInput		= NULL;
	m_lpOutput		= NULL;

	m_pAviSound		= NULL;
	m_pSoundFormat	= NULL;
	m_lpdsbAvi	    = NULL;
	m_dwLoadPos		= 0;
	m_dwLoadSize	= 0;
	m_nSoundFramesAhead = 0;

	SetRect(&m_rcSrc, 0, 0, 0, 0);
	SetRect(&m_rcDst, 0, 0, 0, 0);

	ZeroMemory(&m_stSoundInfo, sizeof(AVISTREAMINFO));
	ZeroMemory(&m_stStreamInfo, sizeof(AVISTREAMINFO));

}


VOID CAvi::Release()
{
	Stop();

	if ( m_Decompressor )
	{
		ICDecompressEnd(m_Decompressor);
		ICClose(m_Decompressor);
		m_Decompressor = 0;
	}
	
	if ( m_pInputFormat ) 
	{
		SAFE_DELETE(m_pInputFormat);
	}

	if ( m_pTargetFormat ) 
	{
		SAFE_DELETE(m_pTargetFormat);
	}

	if ( m_lpInput )
	{
		SAFE_DELETE(m_lpInput);
	}

	if ( m_lpOutput ) 
	{
		SAFE_DELETE(m_lpOutput);
	}

	if ( m_pAviStream ) 
	{
		AVIStreamRelease( m_pAviStream);
		m_pAviStream = NULL;
	}

	if ( m_pAviSound )  
	{
		AVIStreamRelease( m_pAviSound );
		m_pAviSound = NULL;
	}

	if ( m_pAviFile )   
	{
		AVIFileRelease(m_pAviFile);
		m_pAviFile = NULL;
	}

	AVIFileExit();

	if ( m_pSoundFormat ) 
	{
		SAFE_DELETE(m_pSoundFormat);
	}

	if ( m_lpddsAvi ) 
	{
		SAFE_RELEASE(m_lpddsAvi );
	}

	if ( m_lpdsbAvi ) 
	{
		SAFE_RELEASE(m_lpdsbAvi );
	}
}


BOOL CAvi::Create(LPDIRECT3DDEVICE9 lpDD, LPDIRECTSOUND lpDS, const CHAR* pszFile, BOOL bLoop, BOOL bRevPlay)
{
	if ( pszFile == NULL || lpDD == NULL )
	{
		return FALSE;
	}

	Init();

	AVIFileInit();

	if ( AVIFileOpen(&m_pAviFile, pszFile, OF_READ, (LPGUID)&CLSID_AVIFile) )
	{
		AVIFileExit( );	
		return FALSE; 
	}

	if ( AVIFileGetStream(m_pAviFile, &m_pAviStream, streamtypeVIDEO, 0) )
	{	
		AVIFileExit( );
        return FALSE;
	}

	HRESULT hr;
	LONG	lFmtLenght;

	AVIStreamFormatSize(m_pAviStream, 0, &lFmtLenght);
	m_pInputFormat  = (LPBITMAPINFOHEADER) new BYTE[lFmtLenght];
	m_pTargetFormat = (LPBITMAPV4HEADER)   new BYTE[(max(lFmtLenght, sizeof(BITMAPV4HEADER)))];
	ZeroMemory(m_pTargetFormat, sizeof(BITMAPV4HEADER));
	AVIStreamReadFormat(m_pAviStream, 0, m_pInputFormat, &lFmtLenght);
	m_lFrames = AVIStreamLength(m_pAviStream);
	AVIStreamInfo(m_pAviStream, &m_stStreamInfo, sizeof(AVISTREAMINFO));

	hr = lpDD->CreateTexture(
		m_pInputFormat->biWidth,    //±íÃæµÄ¿í
		m_pInputFormat->biHeight,	//±íÃæµÄ¸ß
		0,	//¸ñÊ½
		0,	//ËùÓÃµÄÄÚ´æ³Ø
		D3DFMT_R5G6B5,
		D3DPOOL_MANAGED,
		&m_lpddsAvi,			//±íÃæµÄÖ¸Õë
		NULL				//Ô¤Áô
	);


	if ( hr != S_OK )
	{
		AVIFileExit();
		SAFE_DELETE(m_pInputFormat);
		SAFE_DELETE(m_pTargetFormat);
		return FALSE;
	}

	memcpy(m_pTargetFormat, m_pInputFormat, lFmtLenght);
	
	m_pTargetFormat->bV4Size		  = max(lFmtLenght, sizeof(BITMAPV4HEADER));
	m_pTargetFormat->bV4BitCount = 16;//ddsd.ddpfPixelFormat.dwRGBBitCount;
	m_pTargetFormat->bV4V4Compression = BI_BITFIELDS;
	
	if ( m_pTargetFormat->bV4BitCount == 24 ) 
	{
		m_pTargetFormat->bV4V4Compression = BI_RGB;
	}
	
	m_pTargetFormat->bV4ClrUsed   = 0;
	m_pTargetFormat->bV4RedMask = 0xF800;//ddsd.ddpfPixelFormat.dwRBitMask;
	m_pTargetFormat->bV4GreenMask = 0x7E0;//ddsd.ddpfPixelFormat.dwGBitMask;
	m_pTargetFormat->bV4BlueMask = 0x1F;//ddsd.ddpfPixelFormat.dwBBitMask;
	m_pTargetFormat->bV4AlphaMask = 0;//ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
	
	m_pTargetFormat->bV4SizeImage = ((m_pTargetFormat->bV4Width +3)&0xFFFFFFFC) *
									 m_pTargetFormat->bV4Height * (m_pTargetFormat->bV4BitCount>>3);

	m_lLength = m_pInputFormat->biWidth * m_pInputFormat->biHeight * (m_pInputFormat->biBitCount >> 3);

	if ( m_stStreamInfo.dwSuggestedBufferSize ) 
	{
		m_lLength = (LONG)m_stStreamInfo.dwSuggestedBufferSize;
	}

	m_Decompressor = ICDecompressOpen(ICTYPE_VIDEO,	m_stStreamInfo.fccHandler, m_pInputFormat,
									  (LPBITMAPINFOHEADER)m_pTargetFormat);

	if ( !m_Decompressor ) 
	{
		AVIFileExit();
		SAFE_DELETE(m_pInputFormat);
		SAFE_DELETE(m_pTargetFormat);
		return FALSE;
	}

	m_lpInput  = new BYTE[m_lLength];
	m_lpOutput = new BYTE[m_pTargetFormat->bV4SizeImage];
	ZeroMemory(m_lpInput, m_lLength);
	ZeroMemory(m_lpOutput, m_pTargetFormat->bV4SizeImage);

	m_lLinePitch = m_pTargetFormat->bV4Width * (m_pTargetFormat->bV4BitCount >> 3);
	
	ICDecompressBegin(m_Decompressor, m_pInputFormat, (LPBITMAPINFOHEADER)m_pTargetFormat);
	
	m_dwFps		= m_stStreamInfo.dwRate / m_stStreamInfo.dwScale;	
	m_nTimeTick = (1000 * m_stStreamInfo.dwScale + (m_stStreamInfo.dwRate >> 1)) / m_stStreamInfo.dwRate;

	SetRect(&m_rcSrc, 0, 0, m_pTargetFormat->bV4Width, m_pTargetFormat->bV4Height);
	SetRect(&m_rcDst, 0, 0, 640, 480);

	// »ç¿îµåºÎºÐ ¼³Á¤.(¸¸¾à »ç¿îµå°¡ ¾ø´Ù¸é ÀÌ¹ÌÁö¸¸ ÇÃ·¹ÀÌÇÏ°Ô ¼³Á¤ÇÑ´Ù.)
	AVIFileGetStream(m_pAviFile, &m_pAviSound, streamtypeAUDIO, 0);

	if ( lpDS != NULL && m_pAviSound )
	{
		AVIStreamFormatSize(m_pAviSound, 0, &lFmtLenght);
		m_pSoundFormat = (WAVEFORMATEX*) new BYTE[lFmtLenght];

		AVIStreamReadFormat(m_pAviSound, 0, m_pSoundFormat, &lFmtLenght);
		AVIStreamInfo(m_pAviSound, &m_stSoundInfo, sizeof(AVISTREAMINFO));

		m_dwLoadSize = (m_pSoundFormat->nAvgBytesPerSec + m_dwFps - 1 ) / m_dwFps;

		DSBUFFERDESC dsbd;
		ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
		dsbd.dwSize		   = sizeof(DSBUFFERDESC);
		dsbd.dwFlags	   = 0;
		dsbd.dwReserved	   = 0;
		dsbd.lpwfxFormat   = m_pSoundFormat;
		dsbd.dwBufferBytes = m_stSoundInfo.dwSuggestedBufferSize * m_dwFps * 2;

		if ( lpDS->CreateSoundBuffer(&dsbd, &m_lpdsbAvi, NULL) != DS_OK )
		{
			AVIFileExit();
			SAFE_DELETE(m_pInputFormat);
			SAFE_DELETE(m_pTargetFormat);
			SAFE_DELETE(m_lpInput);
			SAFE_DELETE(m_lpOutput);
			return FALSE;
		}
	}

	m_bLoop	   = bLoop;
	m_bRevPlay = bRevPlay;

	m_lIndex = (!m_bRevPlay ? 0 : m_lFrames-1);

	return TRUE;
}


VOID CALLBACK GetFrameTimer(UINT, UINT, DWORD dwAviSurface, DWORD, DWORD)
{
	((CAvi*)dwAviSurface)->NextFrame();
}


BOOL CAvi::Start()
{
	if ( m_nTimerID == 0 )
	{
		m_nTimerID = timeSetEvent(m_nTimeTick, 5, GetFrameTimer, (ULONG)this, TIME_PERIODIC);
		m_bPlaying = (m_nTimerID != NULL);
		CAvi::ReadFrame(0);
	}

	if ( m_lpdsbAvi )
	{
		m_nSoundFramesAhead = m_stSoundInfo.dwInitialFrames / m_stSoundInfo.dwScale;

		if ( m_lpdsbAvi->Play(0, 0, DSBPLAY_LOOPING) != DS_OK ) 
		{
			return FALSE;
		}
	}

	if ( m_pAviSound )
	{
		if ( !GetSound() ) 
		{
			return FALSE;
		}
	}

	Sleep(50);

	return m_bPlaying;
}


VOID CAvi::Stop()
{
	if ( m_lpdsbAvi )
	{
		m_lpdsbAvi->Stop();
	}

	if ( m_nTimerID )
	{
		timeKillEvent(m_nTimerID);
		m_nTimerID = 0;

		Sleep(50);
	}

	m_bPlaying = FALSE;	
}


VOID CAvi::NextFrame()
{
	m_lIndex = m_lIndex + (!m_bRevPlay ? 1 : -1);

	if ( m_lIndex == (!m_bRevPlay ? m_lFrames : 0) )
	{
		Stop();

		if ( m_bLoop )
		{
			m_nTimerID   = timeSetEvent(m_nTimeTick, 5, GetFrameTimer, (ULONG)this, TIME_PERIODIC);
			m_lIndex	 = (!m_bRevPlay ? 0 : m_lFrames);
			m_dwLoadPos  = 0;
			m_bPlaying   = (m_nTimerID != NULL);

			if ( m_lpdsbAvi )
			{
				m_lpdsbAvi->SetCurrentPosition(0);
				GetSound();
				m_lpdsbAvi->Play(0, 0, DSBPLAY_LOOPING);
			}
		}
	}

	GetSound();
	ReadFrame(m_lIndex);
}


VOID CAvi::ReadFrame(LONG lFrame)
{
	lFrame -= m_nSoundFramesAhead;

	if ( lFrame < 0 ) 
	{
		return;
	}

	if ( lFrame < m_lFrames )
	{
		if ( m_pAviStream )
		{
			AVIStreamRead(m_pAviStream, lFrame, 1, m_lpInput, m_lLength, NULL, NULL);

			if ( m_Decompressor )
			{
				EnterCriticalSection(&m_AccessBuffer);
				ICDecompress(m_Decompressor, 0, m_pInputFormat, m_lpInput, (LPBITMAPINFOHEADER)m_pTargetFormat, m_lpOutput);
				LeaveCriticalSection(&m_AccessBuffer);
			}
		}
	}
}


BOOL CAvi::GetSound()
{
	if ( m_lpdsbAvi )
	{
		HRESULT hr;
		DWORD	dwSize1, dwSize2;
		LPVOID	Data1, Data2;

/*		hr = m_lpdsbAvi->Lock(m_dwLoadPos * m_dwLoadSize, m_dwLoadSize, &Data1, &dwSize1, &Data2, &dwSize2, 0);

		if ( hr != DS_OK )
		{
			return FALSE;
		}

		AVIStreamRead(m_pAviSound, m_lIndex * m_dwLoadSize >> 1, m_dwLoadSize >> 1, Data1, m_dwLoadSize, NULL, NULL);

		hr = m_lpdsbAvi->Unlock(Data1, dwSize1, Data2, dwSize2);

		m_dwLoadPos++;
		m_dwLoadPos %= m_dwFps;
		*/

		hr = m_lpdsbAvi->Lock(m_dwLoadPos * m_stSoundInfo.dwSuggestedBufferSize, m_stSoundInfo.dwSuggestedBufferSize, &Data1, &dwSize1, &Data2, &dwSize2, 0);

		if ( hr != DS_OK )
		{
			return FALSE;
		}

		if ( m_pSoundFormat->nBlockAlign == 0 )
		{
			return FALSE;
		}

		AVIStreamRead(m_pAviSound, 
			          m_lIndex * m_stSoundInfo.dwSuggestedBufferSize / m_pSoundFormat->nBlockAlign,
		              m_stSoundInfo.dwSuggestedBufferSize / m_pSoundFormat->nBlockAlign, 
					  Data1, m_stSoundInfo.dwSuggestedBufferSize, NULL, NULL);

		hr = m_lpdsbAvi->Unlock(Data1, dwSize1, Data2, dwSize2);

		m_dwLoadPos++;
		m_dwLoadPos %= m_dwFps * 2;

		return TRUE;
	}

	return FALSE;
}


/*
HRESULT CAvi::DrawBuffer()
{
	HRESULT		   hr;
	DDSURFACEDESC2 ddsd;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hr = m_lpddsAvi->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

	if ( hr != DD_OK )
	{
		return hr;
	}

	LPBYTE lpSource = m_lpOutput + m_lLinePitch * (m_pTargetFormat->bV4Height - 1);
	LPBYTE lpDest   = (LPBYTE)ddsd.lpSurface;

	if ( hr == DD_OK )
	{
		EnterCriticalSection(&m_AccessBuffer);

		for ( INT nCnt = 0; nCnt < (INT)m_pTargetFormat->bV4Height; nCnt++)
		{
			memcpy(lpDest, lpSource, m_lLinePitch);
			lpDest   += ddsd.lPitch;
			lpSource -= m_lLinePitch;
		}

		LeaveCriticalSection(&m_AccessBuffer);

		hr = m_lpddsAvi->Unlock(NULL);

		while ( hr == DDERR_SURFACELOST ) 
		{
			hr = m_lpddsAvi->Unlock(NULL);
		}
	}

	return hr;
}
*/
HRESULT CAvi::DrawBuffer(VOID)
{
	HRESULT hr;
	D3DLOCKED_RECT ddsd;

	if ( m_lpddsAvi )
	{
		hr = m_lpddsAvi->LockRect(0, &ddsd, 0, 0);
		
		if ( hr == S_OK && m_lpOutput )
		{
			LPBYTE lpSource = m_lpOutput + m_lLinePitch * (m_pTargetFormat->bV4Height - 1);
			LPBYTE lpDest   = (LPBYTE)ddsd.pBits;

			INT xs = m_lLinePitch>>2,
				xm = m_lLinePitch*2,
				y  = m_pTargetFormat->bV4Height,
				z  = ddsd.Pitch - m_lLinePitch;

			EnterCriticalSection(&m_AccessBuffer);

			if ( m_pTargetFormat && m_pTargetFormat->bV4Width % 4)
			{	
				for( INT nCnt = 0; nCnt < (INT)m_pTargetFormat->bV4Height; nCnt++ )
				{	
					memcpy(lpDest, lpSource, m_lLinePitch);
					lpDest   += ddsd.Pitch;
					lpSource -= m_lLinePitch;
				}
			}
			else
			{
				__asm
				{   
					mov ebx, [y]
					mov esi, lpSource
					mov edi, lpDest
					cld

				lp: mov ecx, [xs]
					rep movsd

					sub esi, [xm]
					add edi, [z]

					dec ebx
					jnz lp
				}
			}

			LeaveCriticalSection(&m_AccessBuffer);

			hr = m_lpddsAvi->UnlockRect(0);
		}

		return hr;
	}

	return E_FAIL;
}


BOOL CAvi::Draw(LPDIRECT3DTEXTURE9 lpddsDest)
{
	if ( DrawBuffer() != S_OK)
	{
		return FALSE;
	}
	
		g_pSpirit->Begin(0);
		D3DXMATRIX oldMat;
		D3DXMATRIX mat;
		g_pSpirit->GetTransform(&oldMat);
		FLOAT scaleY = (FLOAT)m_rcDst.bottom / m_rcSrc.bottom;
		D3DXVECTOR2 ve2 = D3DXVECTOR2(1, scaleY);
		D3DXMatrixTransformation2D(&mat, NULL, 0, &ve2, NULL, 0, NULL);
		g_pSpirit->SetTransform(&mat);
		D3DXVECTOR3 ve3 = D3DXVECTOR3(0, 0, 0);
		g_pSpirit->Draw(m_lpddsAvi, NULL, NULL, &ve3, 0xffffffff);
		g_pSpirit->End();
		g_pSpirit->SetTransform(&oldMat);
	//	lpddsDest->Blt(&m_rcDst, m_lpddsAvi, &m_rcSrc, DDBLT_WAIT, NULL);
		return m_bPlaying;
}
