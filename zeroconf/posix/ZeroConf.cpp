#include "OscUdpZeroConfService.h"

#include "dns_sd.h"

#include <string>
#include <iostream>
#include <stdexcept>

#include <pthread.h>

class OscUdpZeroConfService::Implementation
{
    std::string m_name;
	OSCRegisterListener *listener;

    long m_port;
	bool stop;

    DNSServiceRef client;

    typedef union { unsigned char b[2]; unsigned short NotAnInteger; } Opaque16;

    pthread_t thread_;

    static void *ThreadFunc( void* p )
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
				tv.tv_sec = 10000000;
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
        int threadId = pthread_create(&thread_,NULL,ThreadFunc, this);
    }

    void Stop()
    {
		stop=true;
        if( thread_ )
        {
			// fixme : stop thread here
			pthread_cancel(thread_);
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
