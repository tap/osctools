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

#include "OscUdpZeroConfService.h"

#include "dns_sd.h"

#include <string>
#include <iostream>
#include <stdexcept>

#include <windows.h>

class OscUdpZeroConfService::Implementation
{
    std::string m_name;
	OSCRegisterListener *listener;

    long m_port;
	bool stop;

    DNSServiceRef client;

    typedef union { unsigned char b[2]; unsigned short NotAnInteger; } Opaque16;

    HANDLE thread_;

    static DWORD WINAPI ThreadFunc( LPVOID p )
    {
        static_cast<OscUdpZeroConfService::Implementation*>(p)->ThreadTask();
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
			// fixme : stop thread here
            TerminateThread(thread_,0);
			CloseHandle(thread_);
            //thread_ = 0;            

        }
    }

    static void DNSSD_API register_reply(   DNSServiceRef       sdRef, 
                                            DNSServiceFlags     flags, 
                                            DNSServiceErrorType errorCode, 
                                            const char          *name, 
                                            const char          *regtype, 
                                            const char          *domain, 
                                            void                *context )
        {
        // do something with the values that have been registered
        Implementation *self = (Implementation *)context;

        std::cout   << "Got a reply for " 
                    << std::string(name)      << "." 
                    << std::string(regtype) 
                    << std::string(domain)    << ": " 
                    << std::endl;
            switch (errorCode)
            {
            case kDNSServiceErr_NoError:      
				std::cout << "Name now registered and active" << std::endl; 
				
				if(self->listener)
					self->listener->OnRegisterService(name);
					
				break;
            case kDNSServiceErr_NameConflict: std::cout << "Name in use, please choose another" << std::endl; /*error*/ break;
            default:                          std::cout << "Error " << errorCode << std::endl; return;
            }
        }

public:

    Implementation(const char *name_, int port_,OSCRegisterListener *listener)
        :m_name(name_)
        ,m_port(port_)
        ,client(NULL)
        ,thread_(NULL)
		,listener(NULL)	
		,stop(false)
    {
        DNSServiceFlags flags	= 0;		                        // default renaming behaviour 
        uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
        const char *name		= m_name.c_str();                   /* may be NULL */
        const char *regtype		= "_osc._udp"; 
        const char *domain		= "local";		                    /* may be NULL */
        const char *host		= "";		                        /* may be NULL */
        uint16_t PortAsNumber	= m_port;
        Opaque16 registerPort   = { { PortAsNumber >> 8, PortAsNumber & 0xFF } };
        uint16_t txtLen			= 0; 
        const void *txtRecord	= "";		                        /* may be NULL */
        DNSServiceRegisterReply callBack = (DNSServiceRegisterReply)&register_reply;	/* may be NULL */
        void *context			= this;		                        /* may be NULL */

        DNSServiceErrorType result = DNSServiceRegister(&client, flags, interfaceIndex, name, regtype, domain, host, registerPort.NotAnInteger,txtLen, txtRecord, callBack, context);

        if(result != kDNSServiceErr_NoError)
            std::cout << "Can't register Service";
        else
            Start();
    }

    ~Implementation()
    {
        Stop();

        if(client)
            DNSServiceRefDeallocate(client);
    }

};
//-----------------------------------------------------------------------------------------
OscUdpZeroConfService::OscUdpZeroConfService(const char *name_, int port_,OSCRegisterListener *listener)
{
    impl_ = new Implementation(name_,port_,listener);
}
//-----------------------------------------------------------------------------------------
OscUdpZeroConfService::~OscUdpZeroConfService()
{
    delete impl_;
}	
