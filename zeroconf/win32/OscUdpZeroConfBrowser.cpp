/*
	Copyright (c) 2005 Rémy Muller. 
	
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OscUdpZeroConfBrowser.h"

#include <dns_sd.h>

#include <stdexcept>
#include <iostream>
#include <windows.h>

class OscUdpZeroConfBrowser::Implementation
{
    DNSServiceRef client;

    OSCBrowseListener *listener;

    HANDLE thread_;
	bool stop;

    static DWORD WINAPI ThreadFunc( LPVOID p )
    {
        static_cast<OscUdpZeroConfBrowser::Implementation*>(p)->ThreadTask();
        return 0;
    }

	void ThreadTask()
    {
        int dns_sd_fd = DNSServiceRefSockFD(client);
		int nfds = dns_sd_fd+1;
		fd_set readfds;
		struct timeval tv;
		
        while(!stop)
        {
			FD_ZERO(&readfds);
    
            if(client) 
			{
				FD_SET(dns_sd_fd, &readfds);
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				int result = select(nfds,&readfds,NULL,NULL,&tv);
				if(result>0)
				{
					DNSServiceErrorType err = kDNSServiceErr_NoError;
					if(client && FD_ISSET(dns_sd_fd,&readfds)) 	err = DNSServiceProcessResult(client );
					if (err) stop=true;
				}
				else
					stop=true;
			}
			else
				stop=true;
        }
		if(client)
			DNSServiceRefDeallocate(client);    }
	
    void Start()
    {
      DWORD threadId;
	  thread_ = CreateThread(0,0,ThreadFunc, this, 0, &threadId);
    }

    void Stop()
    {
		stop=true;
        Sleep(1);

        if( thread_ )
        {
			/* fixme : stop thread here*/
            TerminateThread(thread_,0);
			CloseHandle(thread_);
            //thread_ = 0;
        }
    }

    static void DNSSD_API browse_reply( DNSServiceRef client, 
                                        const DNSServiceFlags flags,
                                        uint32_t ifIndex, 
                                        DNSServiceErrorType errorCode,
                                        const char *replyName, 
                                        const char *replyType, 
                                        const char *replyDomain,                             
                                        void *context)
    {
        Implementation *self = (Implementation *)context;
        const bool addService = (flags & kDNSServiceFlagsAdd);

        if(!self->listener) return;

        if(addService)
        {
            self->listener->OnAddService(replyName,replyType,replyDomain);
        }
        else
        {
            self->listener->OnRemoveService(replyName,replyType,replyDomain);
        }
    }
public:
    Implementation(OSCBrowseListener *listener)
        :listener(listener)
        ,client(NULL)
        ,thread_(NULL)
		,stop(false)
    {
        DNSServiceErrorType err;
        DNSServiceFlags flags	= 0;		// default renaming behaviour 
        uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 

        err = DNSServiceBrowse(&client, 
                                flags, 
                                interfaceIndex, 
                                "_osc._udp", 
                                "local", 
                                (DNSServiceBrowseReply)&browse_reply, 
                                this);

        if (!client || err != kDNSServiceErr_NoError) 
        { 
            std::cout << "DNSService call failed " << (long int)err << std::endl;
        }
        else
        {
            Start();
        }
    }
 
    ~Implementation()
    {
        Stop();
		
		if(client)
			DNSServiceRefDeallocate(client);        
    }
};

//-----------------------------------------------------------------------------------------
OscUdpZeroConfBrowser::OscUdpZeroConfBrowser(OSCBrowseListener *listener)
{
    impl_ = new Implementation(listener);
}
//-----------------------------------------------------------------------------------------
OscUdpZeroConfBrowser::~OscUdpZeroConfBrowser()
{
    delete impl_;
}
