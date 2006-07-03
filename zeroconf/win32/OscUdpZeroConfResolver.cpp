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

#include "OscUdpZeroConfResolver.h"

#include <dns_sd.h>

#include <stdexcept>
#include <iostream>
#include <windows.h>


class OscUdpZeroConfResolver::Implementation
{
    DNSServiceRef client;

    OSCResolveListener *listener;

    HANDLE thread_;
  	bool stop;

    static DWORD WINAPI ThreadFunc( LPVOID p )
    {
        static_cast<OscUdpZeroConfResolver::Implementation*>(p)->ThreadTask();
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
			DNSServiceRefDeallocate(client);
    }



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
            // should stop thread here too
            TerminateThread(thread_,0);
			CloseHandle(thread_);
            //thread_ = 0;            
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
        Implementation *self = (Implementation *)context;
    	union { uint16_t s; u_char b[2]; } port = { opaqueport };
	    uint16_t PortAsNumber = ((uint16_t)port.b[0]) << 8 | port.b[1];

        if(!self->listener) return;

        self->listener->OnResolveService(fullname,hosttarget,PortAsNumber,txtRecord);

        // Note: When the desired results have been returned, 
        // the client MUST terminate the resolve by calling DNSServiceRefDeallocate().
        self->Stop();
        DNSServiceRefDeallocate(self->client);
        self->client = NULL;
    }
public:
    Implementation( const char *replyName,
                    const char *replyType,
                    const char *replyDomain,
                    OSCResolveListener *listener)
        :listener(listener)
        ,client(NULL)
        ,thread_(NULL)
        ,stop(false)
    {
        DNSServiceErrorType err;
        DNSServiceFlags flags	= 0;		// default renaming behaviour 
        uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 

        err = DNSServiceResolve(&client,
                                flags,
                                interfaceIndex,
                                replyName,
                                replyType,
                                replyDomain,
                                (DNSServiceResolveReply)&resolve_reply,
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
OscUdpZeroConfResolver::OscUdpZeroConfResolver(const char *replyName,
                                               const char *replyType,
                                               const char *replyDomain,
                                               OSCResolveListener *listener)
{
    impl_ = new Implementation(replyName,replyType,replyDomain,listener);
}
//-----------------------------------------------------------------------------------------
OscUdpZeroConfResolver::~OscUdpZeroConfResolver()
{
    delete impl_;
}
