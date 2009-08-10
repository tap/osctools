#include "NetServiceBrowser.h"
#include <cassert>

#include "dns_sd.h"

using namespace ZeroConf;

NetServiceBrowser::NetServiceBrowser()
: mpListener(NULL)
{
}

NetServiceBrowser::~NetServiceBrowser()
{
}

void NetServiceBrowser::setListener(NetServiceBrowserListener *pNetServiceBrowserListener)
{
	ScopedLock lock(mCriticalSection);
	mpListener = pNetServiceBrowserListener;
}

NetServiceBrowserListener* NetServiceBrowser::getListener() const
{
	ScopedLock lock(mCriticalSection);
	return mpListener;
}

void NetServiceBrowser::searchForBrowsableDomains()
{
	assert(0);
}

void NetServiceBrowser::searchForRegistrationDomains()
{
	assert(0);
}

void NetServiceBrowser::searchForServicesOfType(const std::string &serviceType, const std::string &domainName)
{
	assert(0);
}

void NetServiceBrowser::stop()
{
	assert(0);
}

