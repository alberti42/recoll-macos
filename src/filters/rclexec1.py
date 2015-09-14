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

# Common code for replacing the old shell scripts with Python execm
# ones: this implements the basic functions for a filter which
# executes a command to translate a simple file (like rclword with
# antiword).
#
# This was motivated by the Windows port: to replace shell and Unix
# utility (awk , etc usage). We can't just execute python scripts,
# this would be to slow. So this helps implementing a permanent script
# to repeatedly execute single commands.

import subprocess
import rclexecm

# This class has the code to execute the subprocess and call a
# data-specific post-processor. Command and processor are supplied by
# the object which we receive as a parameter, which in turn is defined
# in the actual executable filter (e.g. rcldoc.py)
class Executor:
    opt_ignxval = 1
    
    def __init__(self, em, flt):
        self.em = em
        self.flt = flt
        self.currentindex = 0

    def runCmd(self, cmd, filename, postproc, opt):
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
        if (opt & self.opt_ignxval) == 0 and proc.returncode:
            self.em.rclog("extractone: [%s] returncode %d" % \
                          (filename, proc.returncode))
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
            return (ok, "", "", rclexecm.RclExecM.eofnow)

        fn = params["filename:"]
        while True:
            cmdseq = self.flt.getCmd(fn)
            cmd = cmdseq[0]
            postproc = cmdseq[1]
            opt = cmdseq[2] if len(cmdseq) == 3 else 0
            if cmd:
                ok, data = self.runCmd(cmd, fn, postproc, opt)
                if ok:
                    break
            else:
                break
        if ok:
            return (ok, data, "", rclexecm.RclExecM.eofnext)
        else:
            return (ok, "", "", rclexecm.RclExecM.eofnow)
        
    ###### File type handler api, used by rclexecm ---------->
    def openfile(self, params):
        self.currentindex = 0
        return True

    def getipath(self, params):
        return self.extractone(params)
        
    def getnext(self, params):
        if self.currentindex >= 1:
            return (False, "", "", rclexecm.RclExecM.eofnow)
        else:
            ret= self.extractone(params)
            self.currentindex += 1
            return ret
