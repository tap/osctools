/**
 */

#include "ext.h"				
#include "ext_obex.h"						
#include "NetServiceBrowser.h"
#include "NetService.h"
#include <iostream>

using namespace ZeroConf;

class BrowserListener;

struct zeroconf_browser 
{
	t_object					ob;			// the object itself (must be first)
  NetServiceBrowser *mpNetServiceBrowser;
  BrowserListener *mpBrowserListener;
};

class BrowserListener : public ZeroConf::NetServiceBrowserListener
{
  zeroconf_browser *mpZeroconf_browser;
public:
  BrowserListener(zeroconf_browser *x)
  : mpZeroconf_browser(x)
  {
  }

  virtual void didFindDomain(NetServiceBrowser *pNetServiceBrowser, const std::string &domainName, bool moreDomainsComing) 
  { 
    object_post((t_object*)mpZeroconf_browser, ""); 
  }
  virtual void didRemoveDomain(NetServiceBrowser *pNetServiceBrowser, const std::string &domainName, bool moreDomainsComing) { object_post((t_object*)mpZeroconf_browser, ""); }
  
  virtual void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing) 
  { 
    object_post((t_object*)mpZeroconf_browser, "found service %s %s", pNetService->getName().c_str(), pNetService->getType().c_str()); 
  }
  virtual void didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing) 
  { 
    object_post((t_object*)mpZeroconf_browser, "lost service %s %s", pNetService->getName().c_str(), pNetService->getType().c_str()); 
  }
  
  virtual void willSearch(NetServiceBrowser *pNetServiceBrowser) { object_post((t_object*)mpZeroconf_browser, ""); }
  virtual void didNotSearch(NetServiceBrowser *pNetServiceBrowser) { object_post((t_object*)mpZeroconf_browser, ""); }
  
  virtual void didStopSearch(NetServiceBrowser *pNetServiceBrowser) { object_post((t_object*)mpZeroconf_browser, ""); }
  
    //object_post((t_object *)mpZeroconf_browser, "willPublish" );
};

//------------------------------------------------------------------------------
void *zeroconf_browser_new(t_symbol *s, long argc, t_atom *argv);
void zeroconf_browser_free(zeroconf_browser *x);
void zeroconf_browser_assist(zeroconf_browser *x, void *b, long m, long a, char *s);

t_class *zeroconf_browser_class;

int main(void)
{		
	t_class *c = class_new("zeroconf.browser", (method)zeroconf_browser_new, (method)zeroconf_browser_free, (long)sizeof(zeroconf_browser), 0L, A_GIMME, 0);
	
  class_addmethod(c, (method)zeroconf_browser_assist,			"assist",		A_CANT, 0);  
	
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	zeroconf_browser_class = c;
  
	return 0;
}

void zeroconf_browser_assist(zeroconf_browser *x, void *b, long m, long a, char *s)
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

void zeroconf_browser_free(zeroconf_browser *x)
{
  if(x->mpNetServiceBrowser)
  {
    delete x->mpNetServiceBrowser;
    x->mpNetServiceBrowser = NULL;
  }
  if(x->mpBrowserListener)
  {
    delete x->mpBrowserListener;
    x->mpBrowserListener = NULL;
  }
}

void *zeroconf_browser_new(t_symbol *s, long argc, t_atom *argv)
{
	zeroconf_browser *x = NULL;
  
	if (x = (zeroconf_browser *)object_alloc(zeroconf_browser_class)) 
	{
    x->mpNetServiceBrowser = NULL;
    x->mpBrowserListener = NULL;
    char *type = NULL;
    char *domain = "local";

    switch(argc)
    {
      case 2:
        if(argv[1].a_type == A_SYM)
        {
          domain = atom_getsym(argv+1)->s_name;
        }
      case 1:
        if(argv[0].a_type == A_SYM)
        {
          type = atom_getsym(argv+1)->s_name;
        }
      default:
        break;
    }
    
    if(type != NULL)
    {
      x->mpNetServiceBrowser = new NetServiceBrowser();
      x->mpBrowserListener = new BrowserListener(x);
      x->mpNetServiceBrowser->setListener(x->mpBrowserListener);
      x->mpNetServiceBrowser->searchForServicesOfType(type, domain);        
    }
	}
	return (x);
}
