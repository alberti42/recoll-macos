#!/usr/bin/env python

###########################################
## Generic recoll multifilter communication code
import sys
import os

class RclExecM:
    def __init__(self):
        self.myname = os.path.basename(sys.argv[0])
        self.mimetype = ""

    def rclog(self, s, doexit = 0, exitvalue = 1):
        print >> sys.stderr, "RCLMFILT:", self.myname, ":", s
        if doexit:
            exit(exitvalue)
    # Our worker sometimes knows the mime types of the data it sends
    def setmimetype(self, mt):
        self.mimetype = mt
    def readparam(self):
        s = sys.stdin.readline()
        if s == '':
            self.rclog(": EOF on input", 1, 0)

        s = s.rstrip("\n")

        if s == "":
            return ("","")
        l = s.split()
        if len(l) != 2:
            self.rclog("bad line: [" + s + "]", 1, 1)

        paramname = l[0].lower()
        paramsize = int(l[1])
        if paramsize > 0:
            paramdata = sys.stdin.read(paramsize)
            if len(paramdata) != paramsize:
                self.rclog("Bad read: wanted %d, got %d" %
                      (paramsize, len(paramdata)), 1,1)
        else:
            paramdata = ""
    
        #self.rclog("paramname [%s] paramsize %d value [%s]" %
        #      (paramname, paramsize, paramdata))
        return (paramname, paramdata)

    # Send answer: document, ipath, possible eof.
    def answer(self, docdata, ipath, iseof):

        print "Document:", len(docdata)
        sys.stdout.write(docdata)

        if len(ipath):
            print "Ipath:", len(ipath)
            sys.stdout.write(ipath)

        if len(self.mimetype):
            print "Mimetype:", len(self.mimetype)
            sys.stdout.write(self.mimetype)

        # If we're at the end of the contents, say so
        if iseof:
            print "Eof: 0"
        # End of message
        print
        sys.stdout.flush()
        #self.rclog("done writing data")

    def processmessage(self, processor, params):

        # We must have a filename entry (even empty). Else exit
        if not params.has_key("filename:"):
            self.rclog("no filename ??", 1, 1)

        # If we're given a file name, open it. 
        if len(params["filename:"]) != 0:
            if not processor.openfile(params):
                self.answer("", "", True)
                return
            
        # If we have an ipath, that's what we look for, else ask for next entry
        ipath = ""
        if params.has_key("ipath:") and len(params["ipath:"]):
            ok, data, ipath, eof = processor.getipath(params)
        else:
            ok, data, ipath, eof = processor.getnext(params)
        #self.rclog("processmessage: ok %s eof %s ipath %s"%(ok, eof, ipath))
        if ok:
            self.answer(data, ipath, eof)
        else:
            self.answer("", "", eof)

    # Loop on messages from our master
    def mainloop(self, processor):
        while 1:
            #self.rclog("waiting for command")

            params = dict()

            # Read at most 10 parameters (normally 1 or 2), stop at empty line
            # End of message is signalled by empty paramname
            for i in range(10):
                paramname, paramdata = self.readparam()
                if paramname == "":
                    break
                params[paramname] = paramdata

            # Got message, act on it
            self.processmessage(processor, params)

