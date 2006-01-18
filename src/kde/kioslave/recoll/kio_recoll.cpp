#ifndef lint
static char rcsid[] = "@(#$Id: kio_recoll.cpp,v 1.1 2006-01-18 13:41:11 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <qfile.h>

#include <kglobal.h>
#include <kurl.h>
#include <kinstance.h>
#include <kio/global.h>

#include <errno.h> 

#include "kio_recoll.h"

using namespace KIO;

RecollProtocol::RecollProtocol(const QCString &pool, const QCString &app) 
    : SlaveBase("recoll", pool, app)
{
}

RecollProtocol::~RecollProtocol()
{
}

void RecollProtocol::get(const KURL & url)
{
    fprintf(stderr, "RecollProtocol::get %s\n", url.url().ascii());

    mimeType("text/html");
    QByteArray output;

    QTextStream os(output, IO_WriteOnly );
    os.setEncoding(QTextStream::Latin1); 
    os << 
	"<html><head><title>Recoll:get</title></head>"
	"<body><h1>Un titre!</h1><p>This is RECOLL</p></body></html>";
    
    data(output);
    data(QByteArray());

    fprintf(stderr, "RecollProtocol::get: calling finished\n");
    finished();
}

void RecollProtocol::mimetype(const KURL & /*url*/)
{
  fprintf(stderr, "RecollProtocol::mimetype\n");
  mimeType("text/html");
  finished();
}

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv)
{
  fprintf(stderr, "KIO_RECOLL\n");
  KInstance instance("kio_recoll");

  if (argc != 4)  {
      fprintf(stderr, 
	      "Usage: kio_recoll protocol domain-socket1 domain-socket2\n");
      exit(-1);
  }

  RecollProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  return 0;
}
