#include "OscUdpZeroConfResolver.h"

#include <dns_sd.h>

#include <stdexcept>
#include <iostream>
#include <pthread.h>


class OscUdpZeroConfResolver::Implementation
{
    DNSServiceRef client;

    OSCResolveListener *listener;

    pthread_t thread_;

    static void* ThreadFunc( void *p )
    {
        static_cast<OscUdpZeroConfResolver::Implementation*>(p)->ThreadTask();
        return 0;
    }

    void ThreadTask()
    {
        // no need to loop we finish the thread as soon as we have a reply
        DNSServiceErrorType err = kDNSServiceErr_NoError;

        if(client) 
            err = DNSServiceProcessResult(client );

        if (err) 
        { 
            //fprintf(stderr, "DNSServiceProcessResult returned %d\n", err); 
        }
        Stop(); 
    }

    void Start()
    {
        threadId =  pthread_create(&thread_, NULL, ThreadFunc, this);
    }

    void Stop()
    {
        if( thread_ )
        {
            // should stop thread here too
            thread_ = 0;
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
