/* User defined header */
#include "CPCommTouch.h"


namespace CP
{

CommTouch::CommTouch()
{

}

CommTouch::~CommTouch()
{
}

bool CommTouch::init()
{
	HRESULT result = S_OK;
	result = m_ctLib.Load("asapsdk64.dll");

	if(SUCCEEDED(result))
	{
		//CPLog::debug() << "Load dll done";

		result = m_ctLib.Init("DirTemp=C:\\Windows\\TEMP\\;WebSecZeroLatencyMode=0;ServerAddress=webres%d.nordnet.ctmail.com;LicenseKey=0001O190E1022T01080P:89e568b726b84f608cb6ed5ea6e06fa6;SleepMode=0;");

		if (S_OK != result)
		{
			//CPLog::debug() << "Init Failed " << result;
			return false;
		}

		result = m_ctLib.SaveCache("C:\\Bloat\\urlf.dat");

		if (S_OK != result)
		{
			//CPLog::debug() << "LoadCache failed " << result;
			return false;
		}
	}
	else
	{
		//CPLog::debug() << "Load DLL failed - " << result;
		return false;
	}

	return true;
}

std::vector<unsigned short> CommTouch::classifyURL(std::string const & url)
{
	//CPLog::debug() << "Try classify " << url;
	auto result = m_ctLib.ClassifyUrl(url.c_str());
	std::vector<unsigned short> categoryIDs;

	if(SUCCEEDED(result))
	{
		//CPLog::debug() << "ClassifyUrl Success";

		CONST USHORT * m_puCategories=NULL;
		USHORT nCount = 5;

		m_ctLib.GetCategories(&m_puCategories, &nCount);

		/// debug
		auto temp = std::to_string(nCount);
		temp += " category(s): ";

		for (auto i = 0; i < nCount; ++i)
		{
			temp += std::to_string(m_puCategories[i]);
			temp += "|";
			categoryIDs.push_back(m_puCategories[i]);
		}
		
		temp.pop_back();
		//CPLog::debug() << temp;
	}
	else
	{
		//CPLog::debug() << "ClassifyUrl Failed " << m_ctLib.GetLastError() << " - " << result;
	}

	return categoryIDs;
}

}
