#pragma once
#ifndef CS_COMMTOUCH_LIB
#define CS_COMMTOUCH_LIB

/* Windows header */
#include <Windows.h>

//#define ASAPSDK_API_CALLCONV	_cdecl
typedef void* CSDKMain;
typedef void* CSDKUrlCat;

/// @brief	Wrapper for the CommTouch COMMTOUCH library.
///
/// CSCommTouchLib ct;
///	if(SUCCEEDED(ct.Load("asapsdk64.dll")))
///	{
///		ct.Init("DirTemp=C:\\Windows\\TEMP\\;WebSecZeroLatencyMode=0;ServerAddress=webres%d.nordnet.ctmail.com;"
///			"LicenseKey=0001S001I0070C115F0Y:9aed5dead0274774a9237189c2462855;ProxyAccess=1;ProxyPort=8080;"
///			"ProxyServerAddress=192.168.128.13;ProxyAuth=NoAuth;SleepMode=0");
///		ct.LoadCache("C:\\ProgramData\\CONTRL~1\\urlf.dat");
///		if(SUCCEEDED(ct.ClassifyUrl("http://office2010.microsoft.com")))
///		{
///			CONST USHORT *m_puCategories=NULL;
///			USHORT nCount=5;
///			ct.GetCategories(&m_puCategories, &nCount);
///			ct.CloseCategories();
///		}
///		else
///		{
///			printf("ClassifyUrl error %i",ct.GetLastError());
///		}
///		ct.Reset();
///	}
///	return 0;

#define	CACHEFILENAME							_T("urlf.dat")

class CSCommTouchLib
{
public:

	typedef enum 
	{
		ASAPERR_None					= 0,
		ASAPERR_General					= 1,
		ASAPERR_InvalidConnectionString	= 2,
		ASAPERR_InvalidArgument			= 3,
		ASAPERR_BadMessageFormat		= 100,
		ASAPERR_CommErrorGeneralError	= 200,
		ASAPERR_CommErrorUnableToConnect= 201,
		ASAPERR_CommErrorConnectFailed	= 202,
		ASAPERR_CommErrorCommTimeout	= 203,
		ASAPERR_CommErrorRequestTimeout	= 204,
		ASAPERR_AuthGeneralError		= 300,
		ASAPERR_AuthFailed				= 301,
		ASAPERR_AuthSessionExpired		= 302,
		ASAPERR_ProtocolGeneral			= 400,
	} e_ASAPErrors;


	CSCommTouchLib(void);
	virtual ~CSCommTouchLib(void);

	HRESULT		Load(LPCSTR pszLib);
	void		UnLoad();
	HRESULT		Init(LPCSTR pszConfig);
	HRESULT		LoadCache(LPCSTR pszCacheFilename);
	HRESULT		SaveCache(LPCSTR pszCacheFilename);
	HRESULT		ClassifyUrl(LPCSTR pszUrl);
	HRESULT		GetCategories(const unsigned short** pCategories, unsigned short* Size);
	HRESULT		CloseCategories();
	e_ASAPErrors	GetLastError();
	HRESULT		Reset();


	int			(_cdecl *m_lpfnCSDKMain_LoadWebSecCache) (CSDKMain cmain, const char* sFileName);
	int			(_cdecl *m_lpfnCSDKMain_SaveWebSecCache) (CSDKMain cmain, const char* sFileName);
	CSDKMain	(_cdecl *m_lpfnCSDKMain_Create) (const char* connectionstring);
	int			(_cdecl *m_lpfnCSDKUrlCat_GetCategories) (CSDKUrlCat curlcat, const unsigned short** pCategories, unsigned short* Size);
	CSDKUrlCat	(_cdecl *m_lpfnCSDKMain_ClassifyUrl) (CSDKMain cmain, const char* sUrl);
	int			(_cdecl *m_lpfnCSDKUrlCat_Close) (CSDKUrlCat curlcat);
	int			(_cdecl *m_lpfnCSDKMain_Close) (CSDKMain cmain);
	e_ASAPErrors(_cdecl *m_lpfnCSDKException_GetErrorCode) (void);

private:
	HRESULT		LoadSymbol(void*& pSymbol, LPCSTR lpszSymbolName);
	HMODULE		m_hLib;
	CSDKMain	m_pMain;
	CSDKUrlCat	m_pCat;
};

#endif /* CS_COMMTOUCH_LIB */
