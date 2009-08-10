#include "NetService.h"

#include "dns_sd.h"

#include <cassert>

using namespace ZeroConf;

NetService::NetService(const std::string &domain, const std::string &type, const std::string &name, const int port)
: mDomain(domain)
, mType(type)
, mName(name)
, mPort(port)
, mpListener(NULL)
{
}

NetService::NetService(const std::string &domain, const std::string &type, const std::string &name)
: mDomain(domain)
, mType(type)
, mName(name)
, mPort(-1)
, mpListener(NULL)
{
}

NetService::~NetService()
{
}

void NetService::setListener(NetServiceListener *pNetServiceListener)
{
	ScopedLock lock(mCriticalSection);
	mpListener = pNetServiceListener;
}

NetServiceListener *NetService::getListener() const
{
	ScopedLock lock(mCriticalSection);
	return mpListener; 
}

void NetService::publish()
{
	assert(0);
}

void NetService::publishWitOptions(Options options)
{
	assert(0);
}

void NetService::resolveWithTimeout()
{
	assert(0);
}

void NetService::stop()
{
	assert(0);
}

void NetService::startMonitoring()
{
	assert(0);
}

void NetService::stopMonitoring()
{
	assert(0);
}
