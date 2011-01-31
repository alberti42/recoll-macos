#!/usr/bin/env python

import sys
import string
import glob
import os
import os.path
from zipfile import ZipFile

class European8859TextClassifier:
    def __init__(self, langzip):
        self.langtables = self.readlanguages(langzip)

        # Table to translate from punctuation to spaces
        punct = '''*?[].@+-,#_$%&={};.,:!"''' + "\n\r"
        spaces = ""
        for c in punct:
            spaces += " " 
        self.spacetable = string.maketrans(punct, spaces)

    # Read the languages stopwords lists
    def readlanguages(self, langzip):
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

        # Remove punctuation
        rawtext = rawtext.translate(self.spacetable)
        # Split words
        words = rawtext.split()
        # Count frequencies
        dict = {}
        for w in words:
            dict[w] = dict.get(w, 0) + 1
        # Order word list by frequency
        lfreq = sorted(dict.iteritems(), \
                       key=lambda entry: entry[1], reverse=True)
        # Check the ntest most frequent words against the language lists and
        # chose the best match
        ntest = 10
        maxcount = 0
        maxlang = ""
        maxcode = ""
        for lang,code,lwords in self.langtables:
            count = 0
            for w,c in lfreq[0:ntest]:
                if w in lwords:
                    count += 1
            print "Lang %s code %s count %d" % (lang, code, count)
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
    langszip = os.path.join(dir, 'iso8859stops.zip')

    classifier = European8859TextClassifier(langszip)

    lang,code,count = classifier.classify(rawtext)
    print "Chosen lang/code/matchcount: %s %s %d" % (lang, code, count)
