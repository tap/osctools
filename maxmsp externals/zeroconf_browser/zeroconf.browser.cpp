/*
	Copyright (c) 2009 Remy Muller. 
	
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

#include "ext.h"				
#include "ext_obex.h"						
#include "zeroconf/NetServiceBrowser.h"
#include "zeroconf/NetService.h"
#include <iostream>

using namespace ZeroConf;

class Browser;

struct zeroconf_browser 
{
	t_object ob;			// the object itself (must be first)
	void *out;
	Browser *mpBrowser;
};

class Browser : public NetServiceBrowser, public NetServiceBrowserListener
{
  zeroconf_browser *mpExternal;
	
public:
  Browser(zeroconf_browser *x)
  : mpExternal(x)
  {
	  setListener(this);
  }

private:
  virtual void didFindDomain(NetServiceBrowser *pNetServiceBrowser, const std::string &domainName, bool moreDomainsComing) { }
  virtual void didRemoveDomain(NetServiceBrowser *pNetServiceBrowser, const std::string &domainName, bool moreDomainsComing) { }
  
  virtual void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing) 
  { 
	  t_atom at[1];
	  atom_setsym(at, gensym(const_cast<char*>(pNetService->getName().c_str())));	  
	  outlet_anything(mpExternal->out, gensym("append"), 1, at);
  }
	
  virtual void didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing) 
  { 
	  t_atom at[1];
	  atom_setsym(at, gensym(const_cast<char*>(pNetService->getName().c_str())));	  
	  outlet_anything(mpExternal->out, gensym("delete"), 1, at);
  }
  
  virtual void willSearch(NetServiceBrowser *pNetServiceBrowser) { }
  virtual void didNotSearch(NetServiceBrowser *pNetServiceBrowser) { }  
  virtual void didStopSearch(NetServiceBrowser *pNetServiceBrowser) { }
};

//------------------------------------------------------------------------------
t_class *zeroconf_browser_class;

void zeroconf_browser_browse(zeroconf_browser *x, t_symbol *s, long argc, t_atom *argv)
{
	char *type = NULL;
	char *domain = "local.";
	
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
				type = atom_getsym(argv+0)->s_name;
			}
		default:
			break;
    }
    
    if(type != NULL)
    {
		if(x->mpBrowser)
			delete x->mpBrowser;
		x->mpBrowser = NULL;
		
		x->mpBrowser = new Browser(x);
		x->mpBrowser->searchForServicesOfType(type, domain);
	}
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
  if(x->mpBrowser)
  {
    delete x->mpBrowser;
  }
}

void *zeroconf_browser_new(t_symbol *s, long argc, t_atom *argv)
{
	zeroconf_browser *x = NULL;
  
	if (x = (zeroconf_browser *)object_alloc(zeroconf_browser_class)) 
	{
		x->out = outlet_new(x, NULL);
        x->mpBrowser = NULL;
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
      x->mpBrowser = new Browser(x);
      x->mpBrowser->searchForServicesOfType(type, domain);        
    }
	}
	return (x);
}

int main(void)
{		
	t_class *c = class_new("zeroconf.browser", (method)zeroconf_browser_new, (method)zeroconf_browser_free, (long)sizeof(zeroconf_browser), 0L, A_GIMME, 0);
	
	class_addmethod(c, (method)zeroconf_browser_browse,			"browse",		A_GIMME, 0);  
	class_addmethod(c, (method)zeroconf_browser_assist,			"assist",		A_CANT, 0);  
	
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	zeroconf_browser_class = c;
	
	return 0;
}
