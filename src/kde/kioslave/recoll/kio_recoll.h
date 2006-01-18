#ifndef _RECOLL_H
#define _RECOLL_H

#include <kio/global.h>
#include <kio/slavebase.h>

class RecollProtocol : public KIO::SlaveBase
{
public:
    RecollProtocol( const QCString &pool, const QCString &app );
    virtual ~RecollProtocol();
    virtual void mimetype(const KURL& url);
    virtual void get( const KURL & url );

    //    virtual void listDir( const KURL & url );
    //    virtual void stat( const KURL & url );
};
#endif
