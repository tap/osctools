/**
 */

#include "ext.h"				
#include "ext_obex.h"						
#include "NetService.h"
#include <iostream>

using namespace ZeroConf;

class ServiceListener;

struct zeroconf_service 
{
	t_object					ob;			// the object itself (must be first)
  NetService *mpNetService;
  ServiceListener *mpServiceListener;
};

class ServiceListener : public ZeroConf::NetServiceListener
{
  zeroconf_service *mpZeroconf_service;
public:
  ServiceListener(zeroconf_service *x)
  : mpZeroconf_service(x)
  {
  }
  
  virtual void willPublish(NetService *pNetService)
  {
    object_post((t_object *)mpZeroconf_service, "willPublish" );
  }
  virtual void didNotPublish(NetService *pNetService)
  {
    object_post((t_object *)mpZeroconf_service, "didNotPublish" );
  }
  virtual void didPublish(NetService *pNetService)
  {
    object_post((t_object *)mpZeroconf_service, "Service published: %s %d", pNetService->getName().c_str(), pNetService->getPort());
  }
  virtual void willResolve(NetService *pNetService)
  {
    object_post((t_object *)mpZeroconf_service, "willResolve" );
  }
  virtual void didNotResolve(NetService *NetService)
  {
    object_post((t_object *)mpZeroconf_service, "didNotResolve" );
  }
  virtual void didResolveAddress(NetService *pNetService)
  {
    object_post((t_object *)mpZeroconf_service, "didResolveAddress %s %d", pNetService->getName().c_str(), pNetService->getPort());
  }
  virtual void didUpdateTXTRecordData(NetService *pNetService)
  {
    object_post((t_object *)mpZeroconf_service, "didUpdateTXTRecordData" );
  }   
  virtual void didStop(NetService *pNetService)
  {
    object_post((t_object *)mpZeroconf_service, "didStop" );
  }
};

//------------------------------------------------------------------------------
void *zeroconf_service_new(t_symbol *s, long argc, t_atom *argv);
void zeroconf_service_free(zeroconf_service *x);
void zeroconf_service_assist(zeroconf_service *x, void *b, long m, long a, char *s);

t_class *zeroconf_service_class;

int main(void)
{		
	t_class *c = class_new("zeroconf.service", (method)zeroconf_service_new, (method)zeroconf_service_free, (long)sizeof(zeroconf_service), 0L, A_GIMME, 0);
	
  class_addmethod(c, (method)zeroconf_service_assist,			"assist",		A_CANT, 0);  
	
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	zeroconf_service_class = c;
  
	return 0;
}

void zeroconf_service_assist(zeroconf_service *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) 
  { 
		sprintf(s, "I am inlet %ld", a);
	} 
	else 
  {	
		sprintf(s, "I am outlet %ld", a); 			
	}
}

void zeroconf_service_free(zeroconf_service *x)
{
  if(x->mpNetService)
  {
    delete x->mpNetService;
    x->mpNetService = NULL;
  }
  if(x->mpServiceListener)
  {
    delete x->mpServiceListener;
    x->mpServiceListener = NULL;
  }
}

void *zeroconf_service_new(t_symbol *s, long argc, t_atom *argv)
{
	zeroconf_service *x = NULL;
  
	if (x = (zeroconf_service *)object_alloc(zeroconf_service_class)) 
	{
    x->mpNetService = NULL;
    x->mpServiceListener = NULL;
    if(argc == 4 && argv[0].a_type == A_SYM && argv[1].a_type == A_SYM && argv[2].a_type == A_SYM && argv[3].a_type == A_LONG)
    {
      const char *domain = atom_getsym(argv+0)->s_name;
      const char *type = atom_getsym(argv+1)->s_name;
      const char *name = atom_getsym(argv+2)->s_name;
      const long port = atom_getlong(argv+3);
      x->mpNetService = new NetService(domain, type, name, port);
      x->mpServiceListener = new ServiceListener(x);
      x->mpNetService->setListener(x->mpServiceListener);
      x->mpNetService->publish();        
    }
    else if(argc == 3 && argv[0].a_type == A_SYM && argv[1].a_type == A_SYM && argv[2].a_type == A_SYM)
    {
      const char *domain = atom_getsym(argv+0)->s_name;
      const char *type = atom_getsym(argv+1)->s_name;
      const char *name = atom_getsym(argv+2)->s_name;
      x->mpNetService = new NetService(domain, type, name);
      x->mpServiceListener = new ServiceListener(x);
      x->mpNetService->setListener(x->mpServiceListener);
      x->mpNetService->resolveWithTimeout(10.0);        
    }
    else
    {
      object_post((t_object *)x, "error");
    }
	}
	return (x);
}
