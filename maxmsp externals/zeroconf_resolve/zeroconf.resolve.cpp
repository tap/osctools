/**
 */

#include "ext.h"				
#include "ext_obex.h"						
#include "zeroconf/NetService.h"
#include <iostream>

using namespace ZeroConf;

class ServiceListener;

struct zeroconf_resolve 
{
  t_object  ob;			// the object itself (must be first)
  void *out;
  NetService *mpNetService;
  ServiceListener *mpServiceListener;
};

class ServiceListener : public ZeroConf::NetServiceListener
{
  zeroconf_resolve *mpExternal;
  
public:
  ServiceListener(zeroconf_resolve *x)
  : mpExternal(x)
  {
  }
  
  virtual void willPublish(NetService *pNetService) {}
  virtual void didNotPublish(NetService *pNetService) {}
  virtual void didPublish(NetService *pNetService) {}
  virtual void willResolve(NetService *pNetService) {}
  virtual void didNotResolve(NetService *NetService)
  {
    object_post((t_object *)mpExternal, "didNotResolve" );
  }
  virtual void didResolveAddress(NetService *pNetService)
  {
      t_atom at[1];
	  
      atom_setsym(at,gensym(const_cast<char*>(pNetService->getHostName().c_str())));
      outlet_anything(mpExternal->out, gensym("host"), 1, at);
	  
      atom_setlong(at, pNetService->getPort());
      outlet_anything(mpExternal->out, gensym("port"), 1, at);
  }
  virtual void didUpdateTXTRecordData(NetService *pNetService) {}   
  virtual void didStop(NetService *pNetService) {}
};

//------------------------------------------------------------------------------
void *zeroconf_resolve_new(t_symbol *s, long argc, t_atom *argv);
void zeroconf_resolve_free(zeroconf_resolve *x);
void zeroconf_resolve_assist(zeroconf_resolve *x, void *b, long m, long a, char *s);

t_class *zeroconf_resolve_class;

int main(void)
{		
	t_class *c = class_new("zeroconf.resolve", (method)zeroconf_resolve_new, (method)zeroconf_resolve_free, (long)sizeof(zeroconf_resolve), 0L, A_GIMME, 0);
	
  class_addmethod(c, (method)zeroconf_resolve_assist,			"assist",		A_CANT, 0);  
	
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	zeroconf_resolve_class = c;
  
	return 0;
}

void zeroconf_resolve_assist(zeroconf_resolve *x, void *b, long m, long a, char *s)
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

void zeroconf_resolve_free(zeroconf_resolve *x)
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

void *zeroconf_resolve_new(t_symbol *s, long argc, t_atom *argv)
{
	zeroconf_resolve *x = NULL;
  
	if (x = (zeroconf_resolve *)object_alloc(zeroconf_resolve_class)) 
	{
	 	x->out = outlet_new(x, NULL);
	
    x->mpNetService = NULL;
    x->mpServiceListener = NULL;
	if(argc == 3 && argv[0].a_type == A_SYM && argv[1].a_type == A_SYM && argv[2].a_type == A_SYM)
    {
      const char *name = atom_getsym(argv+0)->s_name;
      const char *type = atom_getsym(argv+1)->s_name;
      const char *domain = atom_getsym(argv+2)->s_name;
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
