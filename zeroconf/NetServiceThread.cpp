#include "NetServiceThread.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <sys/time.h>

using namespace ZeroConf;

NetServiceThread::NetServiceThread(DNSServiceRef dnsServiceRef, double timeOutInSeconds)
: mDNSServiceRef(dnsServiceRef)
, mTimeOut(timeOutInSeconds)
{
}

NetServiceThread::~NetServiceThread()
{
  if(!waitForThreadToExit(100))
  {
    stopThread(1);
  }
  
  if(mDNSServiceRef)
  {
    DNSServiceRefDeallocate(mDNSServiceRef);
  }
}

void NetServiceThread::run()
{
  int dns_sd_fd = DNSServiceRefSockFD(mDNSServiceRef);
  int nfds = dns_sd_fd+1;
  fd_set readfds;
  struct timeval tv;
  long seconds = long(floor(mTimeOut));
  long uSeconds = long(mTimeOut-seconds);
  
  while(!threadShouldExit())
  {
    FD_ZERO(&readfds);
    
    if(mDNSServiceRef) 
    {
      FD_SET(dns_sd_fd, &readfds);
      tv.tv_sec = seconds;
      tv.tv_usec = uSeconds;
      int result = select(nfds,&readfds,NULL,NULL,&tv);
      if(result>0)
      {
        DNSServiceErrorType err = kDNSServiceErr_NoError;
        if(mDNSServiceRef && FD_ISSET(dns_sd_fd, &readfds)) 	
        {
          err = DNSServiceProcessResult(mDNSServiceRef);
        }
        
        if (err)
        {
          setThreadShouldExit();
        }
      }
      else
      {
        setThreadShouldExit();
      }
    }
    else
    {
      setThreadShouldExit();
    }
  }
}
