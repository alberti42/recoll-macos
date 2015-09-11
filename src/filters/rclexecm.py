#################################
# Copyright (C) 2014 J.F.Dockes
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the
#   Free Software Foundation, Inc.,
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
########################################################
## Recoll multifilter communication module and utilities

import sys
import os
import subprocess
import tempfile
import shutil

############################################
# RclExecM implements the
# communication protocol with the recollindex process. It calls the
# object specific of the document type to actually get the data.
class RclExecM:
    noteof  = 0
    eofnext = 1
    eofnow = 2

    noerror = 0
    subdocerror = 1
    fileerror = 2
    
    def __init__(self):
        try:
            self.myname = os.path.basename(sys.argv[0])
        except:
            self.myname = "???"
        self.mimetype = ""

        if os.environ.get("RECOLL_FILTER_MAXMEMBERKB"):
            self.maxmembersize = \
            int(os.environ.get("RECOLL_FILTER_MAXMEMBERKB"))
        else:
            self.maxmembersize = 50 * 1024
        self.maxmembersize = self.maxmembersize * 1024
        if sys.platform == "win32":
            import msvcrt
            msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)

    def rclog(self, s, doexit = 0, exitvalue = 1):
        print >> sys.stderr, "RCLMFILT:", self.myname, ":", s
        if doexit:
            sys.exit(exitvalue)

    # Note: tried replacing this with a multiple replacer according to
    #  http://stackoverflow.com/a/15221068, which was **10 times** slower
    def htmlescape(self, txt):
        # This must stay first (it somehow had managed to skip after
        # the next line, with rather interesting results)
        txt = txt.replace("&", "&amp;")

        txt = txt.replace("<", "&lt;")
        txt = txt.replace(">", "&gt;")
        txt = txt.replace('"', "&quot;")
        return txt

    # Our worker sometimes knows the mime types of the data it sends
    def setmimetype(self, mt):
        self.mimetype = mt

    # Read single parameter from process input: line with param name and size
    # followed by data.
    def readparam(self):
        s = sys.stdin.readline()
        if s == '':
            sys.exit(0)
#           self.rclog(": EOF on input", 1, 0)

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
        #          (paramname, paramsize, paramdata))
        return (paramname, paramdata)

    # Send answer: document, ipath, possible eof.
    def answer(self, docdata, ipath, iseof = noteof, iserror = noerror):

        if iserror != RclExecM.fileerror and iseof != RclExecM.eofnow:
            if isinstance(docdata, unicode):
                self.rclog("GOT UNICODE for ipath [%s]" % (ipath,))
                docdata = docdata.encode("UTF-8")

            print "Document:", len(docdata)
            sys.stdout.write(docdata)

            if len(ipath):
                print "Ipath:", len(ipath)
                sys.stdout.write(ipath)

            if len(self.mimetype):
                print "Mimetype:", len(self.mimetype)
                sys.stdout.write(self.mimetype)

        # If we're at the end of the contents, say so
        if iseof == RclExecM.eofnow:
            print "Eofnow: 0"
        elif iseof == RclExecM.eofnext:
            print "Eofnext: 0"
        if iserror == RclExecM.subdocerror:
            print "Subdocerror: 0"
        elif iserror == RclExecM.fileerror:
            print "Fileerror: 0"
  
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
            try:
                if not processor.openfile(params):
                    self.answer("", "", iserror = RclExecM.fileerror)
                    return
            except Exception, err:
                self.rclog("processmessage: openfile raised: [%s]" % err)
                self.answer("", "", iserror = RclExecM.fileerror)
                return

        # If we have an ipath, that's what we look for, else ask for next entry
        ipath = ""
        eof = True
        self.mimetype = ""
        try:
            if params.has_key("ipath:") and len(params["ipath:"]):
                ok, data, ipath, eof = processor.getipath(params)
            else:
                ok, data, ipath, eof = processor.getnext(params)
        except Exception, err:
            self.answer("", "", eof, RclExecM.fileerror)
            return

        #self.rclog("processmessage: ok %s eof %s ipath %s"%(ok, eof, ipath))
        if ok:
            self.answer(data, ipath, eof)
        else:
            self.answer("", "", eof, RclExecM.subdocerror)

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


####################################################################
# Common code for replacing the shell scripts: this implements the basic
# functions for a filter which executes a command to translate a
# simple file (like rclword with antiword).
#
# This was motivated by the Windows port: to replace shell and Unix
# utility (awk , etc usage). We can't just execute python scripts,
# this would be to slow. So this helps implementing a permanent script
# to repeatedly execute single commands.
#
# This class has the code to execute the subprocess and call a
# data-specific post-processor. Command and processor are supplied by
# the object which we receive as a parameter, which in turn is defined
# in the actual executable filter (e.g. rcldoc)
class Executor:
    def __init__(self, em, flt):
        self.em = em
        self.flt = flt
        self.currentindex = 0

    def runCmd(self, cmd, filename, postproc):
        ''' Substitute parameters and execute command, process output
        with the specific postprocessor and return the complete text.
        We expect cmd as a list of command name + arguments'''

        try:
            fullcmd = cmd + [filename]
            proc = subprocess.Popen(fullcmd,
                                    stdout = subprocess.PIPE)
            stdout = proc.stdout
        except subprocess.CalledProcessError as err:
            self.em.rclog("extractone: Popen(%s) error: %s" % (fullcmd, err))
            return (False, "")
        except OSError as err:
            self.em.rclog("extractone: Popen(%s) OS error: %s" % (fullcmd, err))
            return (False, "")

        for line in stdout:
            postproc.takeLine(line.strip())

        proc.wait()
        if proc.returncode:
            self.em.rclog("extractone: [%s] returncode %d" % (returncode))
            return False, postproc.wrapData()
        else:
            return True, postproc.wrapData()

    def extractone(self, params):
        #self.em.rclog("extractone %s %s" % (params["filename:"], \
        # params["mimetype:"]))
        self.flt.reset()
        ok = False
        if not params.has_key("filename:"):
            self.em.rclog("extractone: no mime or file name")
            return (ok, "", "", RclExecM.eofnow)

        fn = params["filename:"]
        while True:
            cmd, postproc = self.flt.getCmd(fn)
            if cmd:
                ok, data = self.runCmd(cmd, fn, postproc)
                if ok:
                    break
            else:
                break
        if ok:
            return (ok, data, "", RclExecM.eofnext)
        else:
            return (ok, "", "", RclExecM.eofnow)
        
    ###### File type handler api, used by rclexecm ---------->
    def openfile(self, params):
        self.currentindex = 0
        return True

    def getipath(self, params):
        return self.extractone(params)
        
    def getnext(self, params):
        if self.currentindex >= 1:
            return (False, "", "", RclExecM.eofnow)
        else:
            ret= self.extractone(params)
            self.currentindex += 1
            return ret

# Helper routine to test for program accessibility
def which(program):
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)
    def ext_candidates(fpath):
        yield fpath
        for ext in os.environ.get("PATHEXT", "").split(os.pathsep):
            yield fpath + ext

    def path_candidates():
        yield os.path.dirname(sys.argv[0])
        for path in os.environ["PATH"].split(os.pathsep):
            yield path
            
    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in path_candidates():
            exe_file = os.path.join(path, program)
            for candidate in ext_candidates(exe_file):
                if is_exe(candidate):
                    return candidate
    return None

# Temp dir helper
class SafeTmpDir:
    def __init__(self, em):
        self.em = em
        self.toptmp = ""
        self.tmpdir = ""

    def __del__(self):
        try:
            if self.toptmp:
                shutil.rmtree(self.tmpdir, True)
                os.rmdir(self.toptmp)
        except Exception as err:
            self.em.rclog("delete dir failed for " + self.toptmp)

    def getpath(self):
        if not self.tmpdir:
            envrcltmp = os.getenv('RECOLL_TMPDIR')
            if envrcltmp:
                self.toptmp = tempfile.mkdtemp(prefix='rcltmp', dir=envrcltmp)
            else:
                self.toptmp = tempfile.mkdtemp(prefix='rcltmp')

            self.tmpdir = os.path.join(self.toptmp, 'rclsofftmp')
            os.makedirs(self.tmpdir)

        return self.tmpdir
   

# Common main routine for all python execm filters: either run the
# normal protocol engine or a local loop to test without recollindex
def main(proto, extract):
    if len(sys.argv) == 1:
        proto.mainloop(extract)
    else:
        # Got a file name parameter: TESTING without an execm parent
        # Loop on all entries or get specific ipath
        def mimetype_with_file(f):
            cmd = 'file -i "' + f + '"'
            fileout = os.popen(cmd).read()
            lst = fileout.split(':')
            mimetype = lst[len(lst)-1].strip()
            lst = mimetype.split(';')
            return lst[0].strip()
        def mimetype_with_xdg(f):
            cmd = 'xdg-mime query filetype "' + f + '"'
            return os.popen(cmd).read().strip()
        params = {'filename:': sys.argv[1]}
        # Some filters (e.g. rclaudio) need/get a MIME type from the indexer
        mimetype = mimetype_with_xdg(sys.argv[1])
        params['mimetype:'] = mimetype
        if not extract.openfile(params):
            print "Open error"
            sys.exit(1)
        ipath = ""
        if len(sys.argv) == 3:
            ipath = sys.argv[2]

        if ipath != "":
            params['ipath:'] = ipath
            ok, data, ipath, eof = extract.getipath(params)
            if ok:
                print "== Found entry for ipath %s (mimetype [%s]):" % \
                      (ipath, proto.mimetype)
                if isinstance(data, unicode):
                    bdata = data.encode("UTF-8")
                else:
                    bdata = data
                sys.stdout.write(bdata)
                print
            else:
                print "Got error, eof %d"%eof
            sys.exit(0)

        ecnt = 0   
        while 1:
            ok, data, ipath, eof = extract.getnext(params)
            if ok:
                ecnt = ecnt + 1
                print "== Entry %d ipath %s (mimetype [%s]):" % \
                      (ecnt, ipath, proto.mimetype)
                if isinstance(data, unicode):
                    bdata = data.encode("UTF-8")
                else:
                    bdata = data
                sys.stdout.write(bdata)
                print
                if eof != RclExecM.noteof:
                    break
            else:
                print "Not ok, eof %d" % eof
                break
