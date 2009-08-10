#include "NetServiceBrowser.h"
#include "NetService.h"
#include "NetServiceThread.h"
#include <cassert>
#include "dns_sd.h"

using namespace ZeroConf;

static void browse_reply(DNSServiceRef client, 
                         const DNSServiceFlags flags,
                         uint32_t ifIndex,                          
                         DNSServiceErrorType errorCode,
                         const char *replyName, 
                         const char *replyType, 
                         const char *replyDomain,                             
                         void *context)
{
  NetServiceBrowser *self = (NetServiceBrowser *)context;
  const bool addService = flags & kDNSServiceFlagsAdd;
  
  if(!self->getListener()) return;
  
  if(addService)
  {
    self->addService(replyDomain,replyType,replyName);
  }
  else
  {
    self->removeService(replyDomain,replyType,replyName);
  }
}

NetServiceBrowser::NetServiceBrowser()
: mpListener(NULL)
, mpNetServiceThread(NULL)
{
}

NetServiceBrowser::~NetServiceBrowser()
{
  if(mpNetServiceThread)
    stop();
}

void NetServiceBrowser::setListener(NetServiceBrowserListener *pNetServiceBrowserListener)
{
	mpListener = pNetServiceBrowserListener;
}

NetServiceBrowserListener* NetServiceBrowser::getListener() const
{
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
  if(mpNetServiceThread)
    stop();
  
  DNSServiceFlags flags	= 0;		// default renaming behaviour 
  uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
  DNSServiceRef dnsServiceRef;
  DNSServiceErrorType err = DNSServiceBrowse(&dnsServiceRef, 
                                             flags, 
                                             interfaceIndex, 
                                             serviceType.c_str(), 
                                             domainName.c_str(), 
                                             (DNSServiceBrowseReply)&browse_reply, 
                                             this);
  
  if (!dnsServiceRef || err != kDNSServiceErr_NoError) 
  { 
    if(mpListener)
    {
      mpListener->didNotSearch(this);
    }
  }
  else
  {
    if(mpListener)
       mpListener->willSearch(this);
       
    mpNetServiceThread = new NetServiceThread(dnsServiceRef, 0.1);
    mpNetServiceThread->startThread();
  }
}

void NetServiceBrowser::stop()
{
  if(mpNetServiceThread)
  {
    mpNetServiceThread->setThreadShouldExit();
    mpNetServiceThread->waitForThreadToExit(100);
    delete mpNetServiceThread;
    mpNetServiceThread = NULL;
  }
}

void NetServiceBrowser::addService(const char *domain, const char *type, const char *name)
{
  NetService *pNetService = new NetService(domain, type, name);
  {
    ScopedLock lock(mCriticalSection);
    mServices.push_back(pNetService);     
  }
  if(mpListener)
    mpListener->didFindService(this, pNetService, true);
}

void NetServiceBrowser::removeService(const char *domain, const char *type, const char *name)
{
  ScopedLock lock(mCriticalSection);
  for(std::vector<NetService*>::iterator it = mServices.begin(); it != mServices.end();)
  {
       if((*it)->getName() == name && (*it)->getDomain() == domain && (*it)-> getType() == type)
       {
          NetService *pNetService = *it;
          it = mServices.erase(it);
          if(mpListener)
            mpListener->didRemoveService(this, pNetService, true);
       }
       else
       {
        ++it;
       }
  }
}

