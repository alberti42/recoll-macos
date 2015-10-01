#!/usr/bin/env python
from __future__ import print_function

import rclexecm
import sys
import os
import shutil
import platform
import subprocess
import glob

sysplat = platform.system()

if sysplat != "Windows":
    print("rcluncomp.py: only for Windows", file = sys.stderr)

sevenz = rclexecm.which("7z")
if not sevenz:
    print("rcluncomp.py: can't find 7z exe. Maybe set recollhelperpath " \
          "in recoll.conf ?", file=sys.stderr)
    sys.exit(1)
#print("rcluncomp.py: 7z is %s" % sevenz, file = sys.stderr)

# Params: uncompression program, input file name, temp directory.
# We ignore the uncomp program, and always use 7z on Windows

infile = sys.argv[2]
outdir = sys.argv[3]

# There is apparently no way to suppress 7z output. Hopefully the
# possible deadlock described by the subprocess module doc can't occur
# here because there is little data printed. AFAIK nothing goes to stderr anyway
subprocess.check_output([sevenz, "e", "-bd", "-y", "-o" + outdir, infile],
                        stderr = subprocess.PIPE)

outputname = glob.glob(os.path.join(outdir, "*"))
# There should be only one file in there..
print(outputname[0])
