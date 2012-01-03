#!/usr/bin/env python
"""Try to guess a text's language and character set by checking how it matches lists of
common words. This is not a primary method of detection because it's slow and unreliable, but it
may be a help in discrimating, for exemple, before european languages using relatively close
variations of iso-8859.
This is used in association with a zip file containing a number of stopwords list: rcllatinstops.zip

As a note, I am looking for a good iso-8859-7 stop words list for greek, the only ones I found
were utf-8 and there are errors when transcoding to iso-8859-7. I guess that there is something
about Greek accents that I don't know and would enable fixing this (some kind of simplification
allowing transliteration from utf-8 to iso-8859-7). An exemple of difficulty is the small letter
epsilon with dasia (in unicode but not iso). Can this be replaced by either epsilon or epsilon
with acute accent ?
"""

import sys
import string
import glob
import os
import os.path
from zipfile import ZipFile

class European8859TextClassifier:
    def __init__(self, langzip):
        """langzip contains text files. Each text file is named like lang_code.txt
        (ie: french_cp1252.txt) and contains an encoded stop word list for the language"""
        
        self.langtables = self.readlanguages(langzip)

        # Table to translate from punctuation to spaces
        self.punct = '''*?[].@+-,#_$%&={};.,:!"''' + "\n\r"
        spaces = ""
        for c in self.punct:
            spaces += " " 
        self.spacetable = string.maketrans(self.punct, spaces)

    def readlanguages(self, langzip):
        """Extract the stop words lists from the zip file"""
        zip = ZipFile(langzip)
        langfiles = zip.namelist()
        langs = []
        for fn in langfiles:
            text = zip.read(fn)
            words = text.split()
            langcode = os.path.basename(fn)
            langcode = os.path.splitext(langcode)[0]
            (lang,code) = langcode.split('_')
            langs.append((lang, code, words))
        return langs

    def classify(self, rawtext):
        # Note: we can't use an re-based method to split the data because it
        # should be considered binary, not text.
        # Limit to reasonable size.
        if len(rawtext) > 10000:
            i = rawtext.find(" ", 9000)
            if i == -1:
                i = 9000
            rawtext = rawtext[0:i]
        # Remove punctuation
        rawtext = rawtext.translate(self.spacetable)
        # Split words. 
        words = rawtext.split()
        # Count frequencies
        dict = {}
        for w in words:
            dict[w] = dict.get(w, 0) + 1
        # Order word list by frequency
        lfreq = sorted(dict.iteritems(), \
                       key=lambda entry: entry[1], reverse=True)
        # Check the text's ntest most frequent words against the
        # language lists and chose the best match
        ntest = 20
        maxcount = 0
        maxlang = ""
        maxcode = ""
        for lang,code,lwords in self.langtables:
            count = 0
            for w,c in lfreq[0:ntest]:
                #print "testing", w
                if w in lwords:
                    count += 1
            #print "Lang %s code %s count %d" % (lang, code, count)
            if maxcount < count:
                maxlang = lang
                maxcount = count
                maxcode = code
        # If match too bad, default to most common
        if maxcount == 0:
            maxlang,maxcode = ('english', 'cp1252')
        return (maxlang, maxcode, maxcount)


if __name__ == "__main__":
    f = open(sys.argv[1])
    rawtext = f.read()
    f.close()

    dir = os.path.dirname(__file__)
    langszip = os.path.join(dir, 'rcllatinstops.zip')

    classifier = European8859TextClassifier(langszip)

    lang,code,count = classifier.classify(rawtext)
    if count > 0:
        print "%s %s %d" % (code, lang, count)
    else:
        print "UNKNOWN UNKNOWN 0"
        
