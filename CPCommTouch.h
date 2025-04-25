#pragma once
#ifndef CP_COMMTOUCH_H
#define CP_COMMTOUCH_H

/* C++ header */
#include <string>
#include <vector>
#include "SCommTouchLib.h"


namespace CP
{

class CommTouch
{
public:
	CommTouch();
	~CommTouch();

	bool init();
	std::vector<unsigned short> classifyURL(std::string const & url);

private:
	CSCommTouchLib m_ctLib;
};

}

#endif /* CP_COMMTOUCH_H */
