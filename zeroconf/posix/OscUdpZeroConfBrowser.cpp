#include "OscUdpZeroConfBrowser.h"

#include <dns_sd.h>

#include <stdexcept>
#include <iostream>
#include <pthread.h>

class OscUdpZeroConfBrowser::Implementation
{
    DNSServiceRef client;

    OSCBrowseListener *listener;

    pthread_t thread_;
	bool stop;

    static void * ThreadFunc( void* p )
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
        int threadId = pthread_create(&thread_,NULL, ThreadFunc, this);
    }

    void Stop()
    {
		stop=true;
        if( thread_ )
        {
			/* fixme : stop thread here*/
			pthread_cancel(thread_);
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
        const bool addService = flags & kDNSServiceFlagsAdd;

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
