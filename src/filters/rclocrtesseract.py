#!/usr/bin/env python3
#################################
# Copyright (C) 2020 J.F.Dockes
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
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
########################################################

# Running tesseract for Recoll OCR (see rclocr.py)

import os
import sys
import tempfile
import subprocess
import glob

import rclexecm

_mswindows = (sys.platform == "win32")
if _mswindows:
    ocrlangfile = "rclocrlang.txt"
    import platform
    if platform.machine().endswith('64'):
        # See comments in ../windows/mkinstdir.sh about the setup of the poppler installation
        popplerdir = "poppler/Library/bin/"
    else:
        popplerdir = "poppler32/"
else:
    popplerdir = ""
    ocrlangfile = ".rclocrlang"

_okexts = ('.tif', '.tiff', '.jpg', '.png', '.jpeg')

tesseractcmd = None
pdftoppmcmd = None
pdftocairocmd = None

def _deb(s):
    rclexecm.logmsg("rclocrtesseract: %s" % s)

tmpdir = None

def _maybemaketmpdir():
    global tmpdir
    if tmpdir:
        if not tmpdir.vacuumdir():
            _deb("openfile: vacuumdir %s failed" % tmpdir.getpath())
            return False
    else:
        tmpdir = rclexecm.SafeTmpDir("rclocrtesseract")


def cleanocr():
    global tmpdir
    if tmpdir:
        del tmpdir
        tmpdir = None


# Return true if tesseract and the appropriate conversion program for
# the file type (e.g. pdftoppt for pdf) appear to be available
def ocrpossible(config, path):
    # Check for tesseract
    global tesseractcmd
    if not tesseractcmd:
        config.setKeyDir(os.path.dirname(path))
        tesseractcmd = config.getConfParam("tesseractcmd")
        if tesseractcmd:
            # It is very tempting to quote this value, esp. on Windows where it
            # will contain whitespace. There is no chance that an actual
            # command line would have quotes, so unquote it.
            tesseractcmd = tesseractcmd.strip('"')
        else:
            tesseractcmd = rclexecm.which("tesseract")
        if not tesseractcmd:
            _deb("tesseractcmd not found")
            return False
    if not os.path.isfile(tesseractcmd):
        _deb("tesseractcmd parameter [%s] is not a file" % tesseractcmd)
        return False
    
    # Check input format
    base,ext = os.path.splitext(path)
    ext = ext.lower()
    if ext in _okexts:
        return True

    if ext == '.pdf':
        # Check for pdftoppm or pdftocairo. pdftocairo, can produce a multi-page pdf and make the
        # rest simpler, but the legacy code used pdftoppm, and some poppler installs do not include
        # pdftocairo.
        global pdftoppmcmd, pdftocairocmd
        if not pdftoppmcmd and not pdftocairocmd:
            pdftocairocmd = rclexecm.which(popplerdir + "pdftocairo")
            if not pdftocairocmd:
                pdftoppmcmd = rclexecm.which(popplerdir + "pdftoppm")
        if pdftoppmcmd or pdftocairocmd:
            return True

    return False


# Try to guess tesseract language. This should depend on the input
# file, but we have no general way to determine it. So use the
# environment and hope for the best.
def _guesstesseractlang(config, path):
    tesseractlang = ""

    dirname = os.path.dirname(path)

    # First look for a language def file in the file's directory
    pdflangfile = os.path.join(dirname, ocrlangfile)
    if os.path.isfile(pdflangfile):
        tesseractlang = open(pdflangfile, "r").read().strip()
    if tesseractlang:
        _deb("Tesseract lang from file: %s" % tesseractlang)
        return tesseractlang

    # Then look for a config file  option.
    config.setKeyDir(dirname)
    tesseractlang = config.getConfParam("tesseractlang")
    if tesseractlang:
        _deb("Tesseract lang from config: %s" % tesseractlang)
        return tesseractlang

    # Half-assed trial to guess from LANG then default to english
    try:
        localelang = os.environ.get("LANG", "").split("_")[0]
        if localelang == "en":
            tesseractlang = "eng"
        elif localelang == "de":
            tesseractlang = "deu"
        elif localelang == "fr":
            tesseractlang = "fra"
    except:
        pass

    if not tesseractlang:
        tesseractlang = "eng"
    _deb("Tesseract lang (guessed): %s" % tesseractlang)
    return tesseractlang


# Process pdf file: use pdftoppm to split it into ppm pages, then run
# tesseract on each and concatenate the result. It would probably be
# possible instead to use pdftocairo to produce a tiff, buf pdftocairo
# is sometimes not available (windows).
def _pdftesseract(config, path):
    if not tmpdir:
        return b""

    tesseractlang = _guesstesseractlang(config, path)

    #tesserrorfile = os.path.join(tmpdir.getpath(), "tesserrorfile")
    tmpfile = os.path.join(tmpdir.getpath(), "ocrXXXXXX")

    # Split pdf pages
    try:
        tmpdir.vacuumdir()
        if pdftocairocmd:
            cmd = [pdftocairocmd, "-tiff", "-tiffcompression", "lzw", "-r", "300", path, tmpfile]
        else:
            cmd = [pdftoppmcmd, "-r", "300", path, tmpfile]
            #_deb("Executing %s" % cmd)
        subprocess.check_call(cmd)
    except Exception as e:
        _deb("%s failed: %s" % (pdftoppmcmd,e))
        return b""

    # Note: unfortunately, pdftoppm silently fails if the temp file
    # system is full. There is no really good way to check for
    # this. We consider any empty file to signal an error
    
    pages = glob.glob(tmpfile + "*")
    for f in pages:
        size = os.path.getsize(f)
        if os.path.getsize(f) == 0:
            _deb("pdftoppm created empty files. "
                 "Suspecting full file system, failing")
            return False, ""

    nenv = os.environ.copy()
    cnthreads = config.getConfParam("tesseractnthreads")
    if cnthreads:
        try:
            nthreads = int(cnthreads)
            nenv['OMP_THREAD_LIMIT'] = cnthreads
        except:
            pass

    for f in sorted(pages):
        out = b''
        try:
            out = subprocess.check_output(
                [tesseractcmd, f, f, "-l", tesseractlang],
                stderr=subprocess.STDOUT, env=nenv)
        except Exception as e:
            _deb("%s failed: %s" % (tesseractcmd,e))

        errlines = out.split(b'\n')
        if len(errlines) > 5:
            _deb("Tesseract error output: %d %s" % (len(errlines),out))

    # Concatenate the result files
    txtfiles = glob.glob(tmpfile + "*" + ".txt")
    data = b""
    for f in sorted(txtfiles):
        data += open(f, "rb").read()

    return True,data


def _simpletesseract(config, path):
    tesseractlang = _guesstesseractlang(config, path)

    try:
        out = subprocess.check_output(
            [tesseractcmd, path, 'stdout', '-l', tesseractlang],
            stderr=subprocess.DEVNULL)
    except Exception as e:
        _deb("%s failed: %s" % (tesseractcmd,e))
        return False, ""
    return True, out


# run ocr on the input path and output the result data.
def runocr(config, path):
    _maybemaketmpdir()
    base,ext = os.path.splitext(path)
    ext = ext.lower()
    if ext in _okexts:
        return _simpletesseract(config, path)
    else:
        return _pdftesseract(config, path)

   


if __name__ == '__main__':
    import rclconfig
    config = rclconfig.RclConfig()
    path =  sys.argv[1]
    if ocrpossible(config, path):
        ok, data = runocr(config, sys.argv[1])
    else:
        _deb("ocrpossible returned false")
        sys.exit(1)
    if ok:
        sys.stdout.buffer.write(data)
    else:
        _deb("OCR program failed")
