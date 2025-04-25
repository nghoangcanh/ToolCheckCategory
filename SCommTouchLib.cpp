//#include "socle.h"
#include <Windows.h>
#include "SCommTouchLib.h"


CSCommTouchLib::CSCommTouchLib(void)
{
	m_hLib = NULL;
	m_pMain = NULL;
	m_pCat = NULL;
}


CSCommTouchLib::~CSCommTouchLib(void)
{
	Reset();
}

HRESULT CSCommTouchLib::Load( LPCSTR pszLib )
{
	if(m_hLib)
	{
		UnLoad();
	}

	m_hLib = LoadLibraryA(pszLib);
	
	// even if the LoadLibray failed we try to load symbol 
	// to initialize the functions address to 0x0000
	LoadSymbol((void*&)m_lpfnCSDKMain_Create, "CSDKMain_Create");
	LoadSymbol((void*&)m_lpfnCSDKMain_LoadWebSecCache,	"CSDKMain_LoadWebSecCache");
	LoadSymbol((void*&)m_lpfnCSDKMain_SaveWebSecCache,	"CSDKMain_SaveWebSecCache");
	LoadSymbol((void*&)m_lpfnCSDKMain_ClassifyUrl,	"CSDKMain_ClassifyUrl");
	LoadSymbol((void*&)m_lpfnCSDKUrlCat_GetCategories,	"CSDKUrlCat_GetCategories");
	LoadSymbol((void*&)m_lpfnCSDKUrlCat_Close,	"CSDKUrlCat_Close"); 
	LoadSymbol((void*&)m_lpfnCSDKMain_Close, "CSDKMain_Close");
	LoadSymbol((void*&)m_lpfnCSDKException_GetErrorCode, "CSDKException_GetErrorCode");

	if(m_hLib == NULL)
	{
		return HRESULT_FROM_WIN32(::GetLastError());
	}

	return S_OK;
}

void CSCommTouchLib::UnLoad()
{
	if(m_hLib)
	{
		FreeLibrary(m_hLib);
	}

	m_hLib = NULL;
}

HRESULT CSCommTouchLib::LoadCache(LPCSTR pszCacheFilename)
{
	__try
	{
		m_lpfnCSDKMain_LoadWebSecCache(m_pMain,pszCacheFilename);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	if(GetLastError() == ASAPERR_None)
	{
		return S_OK;
	}
	return E_FAIL;
}

HRESULT CSCommTouchLib::LoadSymbol( void*& pSymbol, LPCSTR lpszSymbolName )
{
	if((pSymbol = (void*) GetProcAddress(m_hLib, lpszSymbolName)) == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	}
	return S_OK;
}

HRESULT CSCommTouchLib::Init(LPCSTR pszConfig)
{
	if(m_hLib == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
	}

	if(m_pMain)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
	}

	__try
	{
		m_pMain = m_lpfnCSDKMain_Create(pszConfig);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		m_pMain = NULL;
	}
	
	if(m_pMain == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_READY);
	}

	return S_OK;
}

HRESULT CSCommTouchLib::SaveCache(LPCSTR pszCacheFilename)
{
	if(m_hLib == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
	}


	__try
	{
		m_lpfnCSDKMain_SaveWebSecCache(m_pMain,pszCacheFilename);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	if(GetLastError() == ASAPERR_None)
	{
		return S_OK;
	}
	return E_FAIL;
}

HRESULT CSCommTouchLib::ClassifyUrl( LPCSTR pszUrl )
{
	if(m_hLib == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
	}

	if(m_pMain == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_READY);
	}

	__try
	{
		m_pCat = m_lpfnCSDKMain_ClassifyUrl (m_pMain, pszUrl);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		m_pCat = NULL;
	}

	if(m_pCat == NULL)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CSCommTouchLib::GetCategories( const unsigned short** pCategories, unsigned short* Size )
{
	if(m_hLib == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
	}

	if(m_pCat == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_READY);
	}

	__try
	{
		m_lpfnCSDKUrlCat_GetCategories(m_pCat, pCategories, Size);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CSCommTouchLib::CloseCategories()
{
	if(m_hLib == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
	}

	if(m_pCat)
	{
		m_lpfnCSDKUrlCat_Close(m_pCat);
	}

	m_pCat = NULL;
	return S_OK;
}

HRESULT CSCommTouchLib::Reset()
{
	CloseCategories();

	if(m_pMain)
	{
		m_lpfnCSDKMain_Close(m_pMain);
	}

	m_pMain = NULL;
	return S_OK;
}

CSCommTouchLib::e_ASAPErrors CSCommTouchLib::GetLastError()
{
	if(m_pMain)
	{
		return 	m_lpfnCSDKException_GetErrorCode();
	}

	return ASAPERR_None;
	 
}
