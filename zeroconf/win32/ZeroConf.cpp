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

#include "ZeroConf.h"

#include "dns_sd.h"

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>

#include <windows.h>

typedef union { unsigned char b[2]; unsigned short NotAnInteger; } Opaque16;

class ZeroConf::Implementation
{
    std::vector<DNSServiceRef> clients;
    RegisterListener    *registerListener;
    BrowseListener      *browseListener;
    ResolveListener     *resolveListener;

    volatile bool stop;
    bool running;
    HANDLE thread_;

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

        if(!self->browseListener) return;

        if(addService)
        {
            self->browseListener->OnAddService(replyName,replyType,replyDomain);
        }
        else
        {
            self->browseListener->OnRemoveService(replyName,replyType,replyDomain);
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

        if(!self->resolveListener) return;

        self->resolveListener->OnResolveService(fullname,hosttarget,PortAsNumber,txtRecord);

        // Note: When the desired results have been returned, 
        // the client MUST terminate the resolve by calling DNSServiceRefDeallocate().
        DNSServiceRefDeallocate(self->clients[2]);
        self->clients[2] = NULL;
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
				
				if(self->registerListener)
					self->registerListener->OnRegisterService(name);
					
				break;
            case kDNSServiceErr_NameConflict: std::cout << "Name in use, please choose another" << std::endl; /*error*/ break;
            default:                          std::cout << "Error " << errorCode << std::endl; return;
        }
     }


    static DWORD WINAPI ThreadFunc( LPVOID p )
    {
        static_cast<ZeroConf::Implementation*>(p)->ThreadTask();
        return 0;
    }

    void ThreadTask()
    {
        running=true;
        while(!stop)
        {
            int fdmax = 0;
            fd_set readfds;
            FD_ZERO(&readfds);

            for(int i=0;i<clients.size();++i)
            {
                if(clients[i])
                {
                    int fd = DNSServiceRefSockFD(clients[i]);

                    if(fd>fdmax)
                        fdmax = fd;

                    FD_SET(fd, &readfds);
                }
            }
            int nfds = fdmax+1;	

            struct timeval tv;
            tv.tv_sec = 1; // we need a 1 sec timer to see if there are new file descriptors to watch for
            tv.tv_usec = 0;

            int result = select(nfds,&readfds,NULL,NULL,&tv);

            if(result>0)
            {
                DNSServiceErrorType err = kDNSServiceErr_NoError;

                for(int i=0;i<clients.size();++i)
                {
                    if(clients[i])
                    {
                        if( FD_ISSET( DNSServiceRefSockFD(clients[i]) , &readfds) )	
                        {
                            err = DNSServiceProcessResult(clients[i]);
                        }
                    }
                    if (err) 
                    {
                        DNSServiceRefDeallocate(clients[i]);
                        clients[i] = NULL;
                    }
                }
            }
            else if(result == 0)
            {
                // time out
            }
            else
            {
                stop=true;  
                break;
            }
        }
        running=false;
    }

    void Start()
    {
      DWORD threadId;
      stop = false;
	  thread_ = CreateThread(0,0,ThreadFunc, this, 0, &threadId);
    }

    void Stop()
    {
		stop=true;
        if( thread_ )
        {
			// fixme : stop thread here
            TerminateThread(thread_,0);
			CloseHandle(thread_);
            //thread_ = 0;            
        }
    }
public:

    Implementation()
        :stop(false)
        ,running(false)
        ,thread_(NULL)
        ,browseListener(NULL)
        ,resolveListener(NULL)
        ,registerListener(NULL)
    {
        clients.resize(3,0);
    }

    ~Implementation()
    {
        Stop();
        for(int i=0;i<clients.size();++i)
        {
            if(clients[i])
    			DNSServiceRefDeallocate(clients[i]);
        }
    }
    void Register(  const char *name, 
                    const char *type, 
                    const char *domain, 
                    int         port,
                    RegisterListener *listener)
    {
        registerListener = listener;

        if(clients[0])
    			DNSServiceRefDeallocate(clients[0]);
        clients[0]=NULL;

        DNSServiceFlags flags	= 0;		                        // default renaming behaviour 
        uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
        const char *host		= "";		                        /* may be NULL */
        uint16_t PortAsNumber	= port;
        Opaque16 registerPort   = { { PortAsNumber >> 8, PortAsNumber & 0xFF } };
        uint16_t txtLen			= 0; 
        const void *txtRecord	= "";		                        /* may be NULL */
        DNSServiceRegisterReply callBack = (DNSServiceRegisterReply)&register_reply;	/* may be NULL */
        void *context			= this;		                        /* may be NULL */

        DNSServiceErrorType result = DNSServiceRegister(&clients[0], 
                                                        flags, 
                                                        interfaceIndex, 
                                                        name, 
                                                        type, 
                                                        domain, 
                                                        host, 
                                                        registerPort.NotAnInteger,
                                                        txtLen, 
                                                        txtRecord, 
                                                        callBack, 
                                                        context);

        if(result != kDNSServiceErr_NoError)
            std::cout << "Can't register Service";
        else if(!running) 
            Start();
    }

    void Browse (   const char *type,
                    const char *domain,
                    BrowseListener  *listener)
    {
        browseListener = listener;

        if(clients[1])
    	    DNSServiceRefDeallocate(clients[1]);
        clients[1]=NULL;

        DNSServiceErrorType err;
        DNSServiceFlags flags	= 0;		// default renaming behaviour 
        uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 

        err = DNSServiceBrowse(&clients[1], 
                                flags, 
                                interfaceIndex, 
                                type,  
                                domain, 
                                (DNSServiceBrowseReply)&browse_reply, 
                                this);

        if (!clients[1] || err != kDNSServiceErr_NoError) 
        { 
            std::cout << "DNSService call failed " << (long int)err << std::endl;
        }
        else if(!running) 
            Start();
    }

    void Resolve(   const char *name,
                    const char *type,
                    const char *domain,
                    ResolveListener *listener)
    {
        resolveListener = listener;

        if(clients[2])
    	    DNSServiceRefDeallocate(clients[2]);
        clients[2]=NULL;

        DNSServiceErrorType err;
        DNSServiceFlags flags	= 0;		// default renaming behaviour 
        uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 

        err = DNSServiceResolve(&clients[2],
                                flags,
                                interfaceIndex,
                                name,
                                type,
                                domain,
                                (DNSServiceResolveReply)&resolve_reply,
                                this);

        if (!&clients[2] || err != kDNSServiceErr_NoError) 
        { 
            std::cout << "DNSService call failed " << (long int)err << std::endl;
        }
        else if(!running) 
            Start();
    }
};
//-----------------------------------------------------------------------------------------
ZeroConf::ZeroConf()
{
    impl_ = new Implementation();
}
//-----------------------------------------------------------------------------------------
ZeroConf::~ZeroConf()
{
    delete impl_;
}	
void ZeroConf::Register(const char *name, 
                        const char *type, 
                        const char *domain, 
                        int         port,
                        RegisterListener *listener)
{
    impl_->Register(name,type,domain,port,listener);
}

void ZeroConf::Browse ( const char *type,
                        const char *domain,
                        BrowseListener  *listener)
{
    impl_->Browse(type,domain,listener);
}

void ZeroConf::Resolve( const char *name,
                        const char *type,
                        const char *domain,
                        ResolveListener *listener)
{
    impl_->Resolve(name,type,domain,listener);
}
