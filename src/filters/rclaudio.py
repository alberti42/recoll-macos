#!/usr/bin/env python3

# Audio tag extractor for Recoll, using mutagen

import sys
import os
import rclexecm
from rclbasehandler import RclBaseHandler
import time
import datetime
import re

import rclconfig

try:
    import mutagen
    from mutagen import File
    from mutagen.id3 import ID3, ID3TimeStamp
except:
    print("RECFILTERROR HELPERNOTFOUND python3:mutagen")
    sys.exit(1);


re_pairnum = re.compile('''[([]*([0-9]+),\s*([0-9]+)''')

# The 'Easy' mutagen tags conversions are incomplete. We do it ourselves.
# TPA,TPOS,disc DISCNUMBER/TOTALDISCS
# TRCK,TRK,trkn TRACKNUMBER/TOTALTRACKS
# The conversions here are consistent with the ones in MinimServer (2019-03),
# including the rating stuff and TXXX. Lacking: Itunes '----' handling ?

# The 'GROUP' tag is a specific minimserver tag used to create
# sub-containers inside a folder. We used to use 'CONTENTGROUP' for
# this, which was wrong, the latter is a vaguely defined "music
# category" thing.
tagdict = {
    'ALBUM ARTIST': 'ALBUMARTIST',
    'ALBUM' : 'ALBUM',
    'ALBUMARTIST' : 'ALBUMARTIST',
    'ALBUMARTISTSORT' : 'ALBUMARTISTSORT',
    'ALBUMSORT' : 'ALBUMSORT',
    'ARTIST' : 'ARTIST',
    'ARTISTSORT' : 'ARTISTSORT',
    'BPM' : 'BPM',
    'COM' : 'COMMENT',
    'COMM' : 'COMMENT',
    'COMMENT' : 'COMMENT',
    'COMPILATION' : 'COMPILATION',
    'COMPOSER' : 'COMPOSER',
    'COMPOSERSORT' : 'COMPOSERSORT',
    'CONDUCTOR' : 'CONDUCTOR',
    'CONTENTGROUP' : 'CONTENTGROUP',
    'COPYRIGHT' : 'COPYRIGHT',
    'DATE' : 'DATE',
    'DISCNUMBER' : 'DISCNUMBER',
    'DISCSUBTITLE' : 'DISCSUBTITLE',
    'DISCTOTAL' : 'TOTALDISCS',
    'ENCODEDBY' : 'ENCODEDBY',
    'ENSEMBLE' : 'ORCHESTRA',
    'GENRE' : 'GENRE',
    'GROUP' : 'GROUP',
    'ISRC' : 'ISRC',
    'LABEL' : 'LABEL',
    'LANGUAGE' : 'LANGUAGE',
    'LYRICIST' : 'LYRICIST',
    'LYRICS' : 'LYRICS',
    'MOOD' : 'MOOD',
    'ORCHESTRA' : 'ORCHESTRA',
    'PERFORMER' : 'PERFORMER',
    'POP' : 'RATING1',
    'POPM' : 'RATING1',
    'ORIGINALARTIST' : 'ORIGINALARTIST',
    'ORIGINALDATE' : 'ORIGINALDATE',
    'RELEASEDATE' : 'RELEASEDATE',
    'REMIXER' : 'REMIXER',
    'SUBTITLE' : 'SUBTITLE',
    'TAL' : 'ALBUM',
    'TALB' : 'ALBUM',
    'TBP' : 'BPM',
    'TBPM' : 'BPM',
    'TCM' : 'COMPOSER',
    'TCMP' : 'COMPILATION',
    'TCO' : 'GENRE',
    'TCOM' : 'COMPOSER',
    'TCON' : 'GENRE',
    'TCOP' : 'COPYRIGHT',
    'TCP' : 'COMPILATION',
    'TCR' : 'COPYRIGHT',
    'TDA' : 'DATE',
    'TDAT' : 'DATE',
    'TDOR' : 'ORIGINALDATE',
    'TDRC' : 'DATE',
    'TDRL' : 'RELEASEDATE',
    'TEN' : 'ENCODEDBY',
    'TENC' : 'ENCODEDBY',
    'TEXT' : 'LYRICIST',
    'TIT1' : 'CONTENTGROUP',
    'TIT2' : 'TITLE',
    'TIT3' : 'SUBTITLE',
    'TITLE' : 'TITLE',
    'TITLESORT' : 'TITLESORT',
    'TLA' : 'LANGUAGE',
    'TLAN' : 'LANGUAGE',
    'TMOO' : 'MOOD',
    'TOA' : 'ORIGINALARTIST',
    'TOPE' : 'ORIGINALARTIST',
    'TOR' : 'ORIGINALDATE',
    'TORY' : 'ORIGINALDATE',
    'TOTALDISCS' : 'TOTALDISCS',
    'TOTALTRACKS' : 'TOTALTRACKS',
    'TP1' : 'ARTIST',
    'TP2' : 'ALBUMARTIST',
    'TP3' : 'CONDUCTOR',
    'TP4' : 'REMIXER',
    'TPA' : 'DISCNUMBER',
    'TPB' : 'LABEL',
    'TPE1' : 'ARTIST',
    'TPE2' : 'ALBUMARTIST',
    'TPE3' : 'CONDUCTOR',
    'TPE4' : 'REMIXER',
    'TPOS' : 'DISCNUMBER',
    'TPUB' : 'LABEL',
    'TRACK' : 'TRACKNUMBER',
    'TRACKNUM' : 'TRACKNUMBER',
    'TRACKNUMBER' : 'TRACKNUMBER',
    'TRACKTOTAL' : 'TOTALTRACKS',
    'TRC' : 'ISRC',
    'TRCK' : 'TRACKNUMBER',
    'TRDA' : 'DATE',
    'TRK' : 'TRACKNUMBER',
    'TS2' : 'ALBUMARTISTSORT',
    'TSA' : 'ALBUMSORT',
    'TSC' : 'COMPOSERSORT',
    'TSO2' : 'ALBUMARTISTSORT',
    'TSOA' : 'ALBUMSORT',
    'TSOC' : 'COMPOSERSORT',
    'TSOP' : 'ARTISTSORT',
    'TSOT' : 'TITLESORT',
    'TSP' : 'ARTISTSORT',
    'TSRC' : 'ISRC',
    'TSST' : 'DISCSUBTITLE',
    'TST' : 'TITLESORT',
    'TT1' : 'CONTENTGROUP',
    'TT2' : 'TITLE',
    'TT3' : 'SUBTITLE',
    'TXT' : 'LYRICIST',
    'TXXX:ORCHESTRA' : 'ORCHESTRA',
    'TXX:ORCHESTRA' : 'ORCHESTRA',
    'TYE' : 'DATE',
    'TYER' : 'DATE',# wikipedia id3: YEAR
    'ULT' : 'LYRICS',
    'USLT' : 'LYRICS',
    'YEAR' : 'DATE',
    'aART' : 'ALBUMARTIST',
    'cond' : 'CONDUCTOR',
    'cpil' : 'COMPILATION',
    'cprt' : 'COPYRIGHT',
    'disk' : 'DISCNUMBER',
    'gnre' : 'GENRE',
    'labl' : 'LABEL',
    'soaa' : 'ALBUMARTISTSORT',
    'soal' : 'ALBUMSORT',
    'soar' : 'ARTISTSORT',
    'soco' : 'COMPOSERSORT',
    'sonm' : 'TITLESORT',
    'tmpo' : 'BPM',
    'trkn' : 'TRACKNUMBER',
    '\xa9ART' : 'ARTIST',
    '\xa9alb' : 'ALBUM',
    '\xa9cmt' : 'COMMENT',
    '\xa9con' : 'CONDUCTOR',
    '\xa9day' : 'DATE',
    '\xa9gen' : 'GENRE',
    '\xa9grp' : 'CONTENTGROUP',
    '\xa9lyr' : 'LYRICS',
    '\xa9nam' : 'TITLE',
    '\xa9ope' : 'ORIGINALARTIST',
    '\xa9too' : 'ENCODEDBY',
    '\xa9wrt' : 'COMPOSER',
    }

def tobytes(s):
    if type(s) == type(b''):
        return s
    if type(s) != type(''):
        s = str(s)
    return s.encode('utf-8', errors='replace')
    
# mp3:      album, title, artist, genre, date, tracknumber
# flac:     album, title, artist, genre, xxx, tracknumber
# oggvorbis:album, title, artist, genre, date, tracknumber
class AudioTagExtractor(RclBaseHandler):

    def __init__(self, em):
        super(AudioTagExtractor, self).__init__(em)
        config = rclconfig.RclConfig()
        tagfixerfn = config.getConfParam("audiotagfixerscript")
        self.tagfix = None
        if tagfixerfn:
            import runpy
            try:
                d = runpy.run_path(tagfixerfn)
                self.tagfix = d['tagfix']
                self.tagfix()
            except Exception as ex:
                self.em.rclog("tagfix script import failed: %s" % ex)
                pass
            
    def _showMutaInfo(self, mutf):
        self.em.rclog("%s" % mutf.info.pprint())
        for prop in dir(mutf.info):
            self.em.rclog("mutinfo: %s -> %s" %
                          (prop, getattr( mutf.info, prop)))


    def _fixrating(self, minf):
        if 'RATING1' in minf:
            if not 'RATING' in minf:
               val = int(minf['RATING1']) // 51 + 1
               if val > 5:
                   val = 5
               if val < 1:
                   val = 1
               minf['RATING'] = str(val)
            del minf['RATING1']


    def _embeddedImageFormat(self, mutf):
        #self.em.rclog("_embeddedImage: MIME: %s"%mutf.mime)
        try:
            # This fails if we're passed a mutagen.ID3 instead of File
            mime = mutf.mime
        except:
            return ''

        if 'audio/mp3' in mime:
            for tagname in mutf.keys():
                if tagname.startswith('APIC:'):
                    #self.em.rclog("mp3 img: %s" % mutf[tagname].mime)
                    return 'jpg' if mutf[tagname].mime == 'image/jpeg' else 'png'
        elif 'audio/flac' in mime:
            if mutf.pictures:
                return 'jpg' if mutf.pictures[0].mime == 'image/jpeg' else 'png'
        elif 'audio/mp4' in mime:
            if 'covr' in mutf.keys():
                format = mutf['covr'][0].imageformat 
                if format == mutagen.mp4.AtomDataType.JPEG:
                    return 'jpg'
                else:
                    return 'png'
        return ''

    # Date formats found in actual files (any date field): [1961] [1967-01-01]
    #  [1996-11-04T08:00:00Z] [] [0000]  [1994-08-08 07:00]
    # We don't try to process the time part.
    # The method translates the date into a Unix timestamp
    # which means possible trouble for pre-1970 recordings (negative time).
    # Oldest possible date with 32 bits time stamp is 1901, which is ok though.
    #
    # Previous recoll versions had an alias from date to dmtime, which
    # was wrong, because dmtime is the unix integer time. We have
    # removed the alias, and set dmtime from the parsed date value.
    def parsedate(self, dt):
        try:
            dt = dt.decode('utf-8', errors='ignore')
            if len(dt) > 10:
                dt = dt[0:10]
            l = dt.split('-')
            if len(l) > 3 or len(l) == 2 or len(l[0]) != 4 or l[0] == '0000':
                return ''
            if len(l) == 1:
                pdt = datetime.datetime.strptime(dt, "%Y")
            elif len(l) == 3:
                pdt = datetime.datetime.strptime(dt, "%Y-%m-%d")
            val = time.mktime(pdt.timetuple())
            return b"%d" % val
        except:
            return b"0"

    ###################
    # Extract audio parameters from mutagen output. Not all file types supply all or even use the
    # same property names. Translate to consistent str keys and encoded values into our
    # fields dict
    def _extractaudioparams(self, filename, minf, mutf):
        for prop,dflt in [('sample_rate', 44100), ('channels', 2), ('length', 0), ('bitrate', 0)]:
            try:
                minf[prop] = getattr(mutf.info, prop)
            except Exception as e:
                #self.em.rclog("NO %s prop: %s" % (prop, e))
                minf[prop] = dflt

        if minf['bitrate'] == 0 and minf['length'] > 0:
            br = int(os.path.getsize(filename)* 8 / minf['length'])
            minf['bitrate'] = br

        minf['duration'] = minf['length']
        del minf['length']

        # Bits/samp is named sample_size or bits_per_sample (depend on file tp)
        try:
            minf['bits_per_sample'] = getattr(mutf.info, 'bits_per_sample')
        except:
            try:
                minf['bits_per_sample'] = getattr(mutf.info, 'sample_size')
            except:
                #self.em.rclog("using default bits_per_sample")
                minf['bits_per_sample'] = 16

        for tag,val in minf.items():
            minf[tag] = tobytes(val)
        

    # Translate native tag name to our filetype-independant ones. Mostly uses the big tagdict
    # dictionary, with other adjustments.
    def _fixtagname(self, tag):
        # Variations on the COMM tag:
        #   - "COMM::eng" We don't know what to do with the language (or possible other attributes),
        #     so get rid of it for now.
        #   - Also possible: "COMM:ID3v1 Comment:eng" "COMM:iTunNORM:eng" "COMM:Performers:eng"
        if tag.find("COMM:") == 0:
            tag = "COMM"
        #     TXXX:TOTALTRACKS TXXX:ORCHESTRA
        elif tag.find('TXXX:') == 0:
            tag = tag[5:]
        elif tag.find('TXX:') == 0:
            tag = tag[4:]

        if tag.upper() in tagdict:
            tag = tag.upper()

        if tag in tagdict:
            #self.em.rclog("Original tag: <%s>, type0 %s val <%s>" % (tag, type(val), val))
            ntag = tagdict[tag].lower()
            #self.em.rclog("New tag: %s" % ntag)
        else:
            if not tag.isalnum():
                return None
            ntag = tag.lower()
            #self.em.rclog(f"Processing unexpected tag: {tag}, value {val}")
        return ntag


    # Disc and track numbers are special, and output in varying ways.
    def _processdiscortracknumber(self, minf, ntag, val):
        # TPA,TPOS,disc DISCNUMBER/TOTALDISCS
        # TRCK,TRK,trkn TRACKNUMBER/TOTALTRACKS
        for what in ("disc", "track"):
            k = what + "number"
            if ntag == k:
                if isinstance(val, list):
                    val = val[0]
                if not isinstance(val, tuple):
                    val = str(val)
                    mo = re_pairnum.match(val)
                    if mo:
                        val = (mo.group(1), mo.group(2))
                    else:
                        val = val.split('/')
                else:
                    #self.em.rclog(f"{k} : tuple: {val} tp1 {type(val[0])} tp2 {type(val[1])}")
                    pass
                minf[k] = tobytes(val[0])
                if len(val) == 2 and val[1] != 0:
                    k1 = "total" + what + "s"
                    #self.em.rclog(f"Setting {k1} : {val[1]}")
                    minf[k1] = tobytes(val[1])
                #self.em.rclog(f"{k} finally: {minf[k]}")
                return True
        return False
    

    def html_text(self, filename):
        #self.em.rclog(f"processing {filename}")
        if not self.inputmimetype:
            raise Exception("html_text: input MIME type not set")
        mimetype = self.inputmimetype

        # We actually output text/plain
        self.outputmimetype = 'text/plain'

        mutf = None
        strex = ""
        try:
            mutf = File(filename)
        except Exception as ex:
            strex = str(ex)
            try:
                mutf = ID3(filename)
            except Exception as ex:
                strex += str(ex)
        if not mutf:
            # Note: mutagen will fail the open (and raise) for a valid
            # file with no tags. Maybe we should just return an empty
            # text in this case? We seem to get an empty str(ex) in
            # this case, and a non empty one for, e.g. permission
            # denied, but I am not sure that the emptiness will be
            # consistent for all file types. The point of detecting
            # this would be to avoid error messages and useless
            # retries.
            if not strex:
                return b''
            else:
                raise Exception("Open failed: %s" % strex)
        
        #self._showMutaInfo(mutf)

        # The field storage dictionary
        minf = {}
        self._extractaudioparams(filename, minf, mutf)
        
        ####################
        # Metadata tags. 
        for tag,val in mutf.items():
            #self.em.rclog(f"TAG <{tag}> VALUE <{val}>")
            # The names vary depending on the file type. Make them consistant
            ntag = self._fixtagname(tag)
            if not ntag:
                continue
            #self.em.rclog(f"Translated tag: <{ntag}>")

            if self._processdiscortracknumber(minf, ntag, val):
                continue

            try:
                # id3v2.4 can send multiple values as 0-separated strings. Change this to a
                # list. mutagen should probably do this for us... Some file types do return lists
                # of values, e.g. FLAC.
                if not isinstance(val, (list, tuple)):
                    val = str(val)
                    val = val.split("\x00")

                # Change list to string for sending up to recoll. 
                try:
                    if isinstance(val, (list, tuple,)):
                        if isinstance(val[0], str):
                            val = " | ".join(val)
                        else:
                            # Usually mp4.MP4Cover: ignore
                            #self.em.rclog(f"Got value for {ntag} which is list of "
                            #              f"non-strings: {type(val[0])}")
                            continue
                except Exception as err:
                    self.em.rclog(f"Trying list join: {err} for {filename}")

                #self.em.rclog(f"VAL NOW <{val}>")
                val =  tobytes(val)

                if ntag in minf:
                    # Note that it would be nicer to use proper CSV quoting
                    minf[ntag] += b"," + val
                else:
                    minf[ntag] = val
                #self.em.rclog(f"Tag <{ntag}> -> <{val}>")

            except Exception as err:
                self.em.rclog(f"tag: {tag} {minf[ntag]}: {err}. fn {filename}")

        self._fixrating(minf)
        
        #self.em.rclog("minf after extract %s\n" % minf)


        if 'orchestra' in minf:
            val = minf['orchestra']
            if val.startswith(b'orchestra='):
                minf['orchestra'] = val[10:]
                
        #self.em.rclog(f"minf after tags {minf}")

        # Check for embedded image. We just set a flag.
        embdimg = self._embeddedImageFormat(mutf)
        if embdimg:
            #self.em.rclog("Embedded image format: %s" % embdimg)
            minf['embdimg'] = tobytes(embdimg)
        
        self.em.setfield("charset", b'utf-8')
        if self.tagfix:
            self.tagfix(minf)

        if 'date' in minf:
            uxtime = self.parsedate(minf['date'])
            if uxtime:
                minf['dmtime'] = uxtime
                
        for tag,val in minf.items():
            #self.em.rclog("%s -> %s" % (tag, val))
            self.em.setfield(tag, val)
            # Compat with old version
            if tag == 'artist':
                self.em.setfield('author', val)    

        #################
        # Document text: use the mutagen pprint function. The values may be somewhat
        # different from what is in the metadata above because of the corrections we apply or
        # different format choices.
        try:
            docdata = tobytes(mutf.pprint())
        except Exception as err:
            docdata = ""
            self.em.rclog("Doc pprint error: %s" % err)

        return docdata


def makeObject():
    print("makeObject");
    proto = rclexecm.RclExecM()
    print("makeObject: rclexecm ok");
    extract = AudioTagExtractor(proto)
    return 17


if __name__ == '__main__':
    proto = rclexecm.RclExecM()
    extract = AudioTagExtractor(proto)
    rclexecm.main(proto, extract)
