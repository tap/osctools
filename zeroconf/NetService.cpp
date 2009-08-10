#include "NetService.h"
#include "NetServiceThread.h"
#include "Thread.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <sys/time.h>
#include "dns_sd.h"

using namespace ZeroConf;

typedef union { unsigned char b[2]; unsigned short NotAnInteger; } Opaque16;
typedef union { unsigned short NotAnInteger; unsigned char b[2]; } Opaque16b;

static void register_reply(DNSServiceRef       sdRef, 
                           DNSServiceFlags     flags, 
                           DNSServiceErrorType errorCode, 
                           const char          *name, 
                           const char          *regtype, 
                           const char          *domain, 
                           void                *context )
{
  // do something with the values that have been registered
  NetService *self = (NetService *)context;
    
  switch (errorCode)
  {
    case kDNSServiceErr_NoError:      
      if(self->getListener())
      {
        self->getListener()->didPublish(self);
      }      
      break;
    case kDNSServiceErr_NameConflict: 
      if(self->getListener())
      {
        self->getListener()->didNotPublish(self);
      }
      break;
    default:                          
      if(self->getListener())
      {
        self->getListener()->didNotPublish(self);
      }
      return;
  }
}  

static void DNSSD_API resolve_reply(DNSServiceRef client, 
                                    const DNSServiceFlags flags, 
                                    uint32_t ifIndex, 
                                    DNSServiceErrorType errorCode,
                                    const char *fullname, 
                                    const char *hosttarget, 
                                    uint16_t opaqueport, 
                                    uint16_t txtLen, 
                                    const char *txtRecord, 
                                    void *context)
{
  NetService *self = (NetService *)context;
  
  switch (errorCode)
  {
    case kDNSServiceErr_NoError:      
    {   
      Opaque16b port = { opaqueport };
      uint16_t PortAsNumber = ((uint16_t)port.b[0]) << 8 | port.b[1];
      self->setPort(PortAsNumber);
        
      if(self->getListener()) 
      {  
        self->getListener()->didResolveAddress(self);
      }
      break;
    }
    default:
      break;
  }
  
  // Note: When the desired results have been returned, 
  // the client MUST terminate the resolve by calling DNSServiceRefDeallocate().
  self->stop();    
}

//------------------------------------------------------------------------------
NetService::NetService(const std::string &domain, const std::string &type, const std::string &name, const int port)
: mDomain(domain)
, mType(type)
, mName(name)
, mPort(port)
, mpListener(NULL)
, mpNetServiceThread(NULL)
{
}

NetService::NetService(const std::string &domain, const std::string &type, const std::string &name)
: mDomain(domain)
, mType(type)
, mName(name)
, mPort(-1)
, mpListener(NULL)
, mpNetServiceThread(NULL)
{
}

NetService::~NetService()
{
  if(mpNetServiceThread)
  {
    stop();
  }
}

void NetService::setListener(NetServiceListener *pNetServiceListener)
{
	mpListener = pNetServiceListener;
}

NetServiceListener *NetService::getListener() const
{
	return mpListener; 
}

void NetService::setPort(int port)
{
  mPort = port;
}

void NetService::setName(const std::string &name)
{
  mName = name;
}

void NetService::publish()
{
  publishWithOptions(Options(0));
}

void NetService::publishWithOptions(Options options)
{
  if(mpNetServiceThread)
  {
    stop();
  }
  
  if(mPort<0)
  {
    if(mpListener)
    {
      mpListener->didNotPublish(this);
    }
    return;
  } 
    
  DNSServiceFlags flags	= options;		                 // default renaming behaviour 
  uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
  const char *name		= mName.c_str();                   /* may be NULL */
  const char *regtype		= mType.c_str(); 
  const char *domain		= mDomain.c_str();		        /* may be NULL */
  const char *host		= "";		                        /* may be NULL */
  uint16_t PortAsNumber	= mPort;
  Opaque16 registerPort   = { { PortAsNumber >> 8, PortAsNumber & 0xFF } };
  uint16_t txtLen			= 0; 
  const void *txtRecord	= "";		                        /* may be NULL */
  DNSServiceRegisterReply callBack = (DNSServiceRegisterReply)&register_reply;	/* may be NULL */
  void *context			= this;		                        /* may be NULL */
  DNSServiceRef dnsServiceRef = NULL;
  DNSServiceErrorType result = DNSServiceRegister(&dnsServiceRef, 
                                                  flags, 
                                                  interfaceIndex, 
                                                  name, 
                                                  regtype, 
                                                  domain, 
                                                  host, 
                                                  registerPort.NotAnInteger,
                                                  txtLen, 
                                                  txtRecord, 
                                                  callBack, 
                                                  context);
  if(result != kDNSServiceErr_NoError)
  {
    if(mpListener)
    {
      mpListener->didNotPublish(this);
    }
  }
  else
  {
    if(mpListener)
    {
      mpListener->willPublish(this);
    }
    mpNetServiceThread = new NetServiceThread(dnsServiceRef, 0.1);
    mpNetServiceThread->startThread();
  }  
}

void NetService::resolveWithTimeout(double timeOutInSeconds)
{
  if(mpNetServiceThread)
    stop();

  DNSServiceFlags flags	= 0;
  uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
  DNSServiceRef dnsServiceRef;
  DNSServiceErrorType err = DNSServiceResolve(&dnsServiceRef,
                                              flags,
                                              interfaceIndex,
                                              mName.c_str(),
                                              mType.c_str(),
                                              mDomain.c_str(),
                                              (DNSServiceResolveReply)&resolve_reply,
                                              this);
  
  if (!dnsServiceRef || err != kDNSServiceErr_NoError) 
  { 
    if(mpListener)
    {
      mpListener->didNotResolve(this);
    }
  }
  else
  {
    if(mpListener)
    {
      mpListener->willResolve(this);
    }
    mpNetServiceThread = new NetServiceThread(dnsServiceRef, 0.1);
    mpNetServiceThread->startThread();
  }
}

void NetService::stop()
{
  if(mpNetServiceThread)
  {
    mpNetServiceThread->setThreadShouldExit();
    mpNetServiceThread->waitForThreadToExit(100);
    delete mpNetServiceThread;
    mpNetServiceThread = NULL;
  }
}

void NetService::startMonitoring()
{
	assert(0);
}

void NetService::stopMonitoring()
{
	assert(0);
}
