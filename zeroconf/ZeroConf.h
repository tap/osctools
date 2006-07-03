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

#ifndef ZeroConf_H
#define ZeroConf_H

class RegisterListener
{
public:
	virtual void OnRegisterService(const char *name) = 0;
};

class BrowseListener
{
public:
    virtual void OnAddService(const char *name,const char *type,const char *domain) = 0;
    virtual void OnRemoveService(const char *name,const char *type,const char *domain) = 0;
};

class ResolveListener
{
public:
    virtual void OnResolveService(const char *fullName,const char *hostTarget,int port,const char *txtRecord) = 0;
};

class ZeroConf
{
    class Implementation;
    Implementation *impl_;

public:
	ZeroConf();
   	virtual ~ZeroConf();

    void Register(  const char *name, 
                    const char *type, 
                    const char *domain, 
                    int         port,
                    RegisterListener *listener);

    void Browse (   const char *type,
                    const char *domain,
                    BrowseListener  *listener);

    void Resolve(   const char *name,
                    const char *type,
                    const char *domain,
                    ResolveListener *listener);
};

#endif