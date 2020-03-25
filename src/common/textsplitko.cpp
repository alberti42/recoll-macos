/* Copyright (C) 2020 J.F.Dockes
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// Specialized Korean text splitter using konlpy running in a Python
// subprocess. konlpy can use several different backends. We support
// Okt (Twitter) and Mecab at this point. Unfortunately the different
// backends have different POS TAG names, so that things are not
// completly transparent when using another (need to translate the tag
// names in the Python program).

#include "autoconfig.h"

#include <iostream>
#include <string>
#include <cstring>
#include <unordered_set>
#include <mutex>

#include "textsplit.h"
#include "log.h"
//#define UTF8ITER_CHECK
#include "utf8iter.h"
#include "smallut.h"
#include "rclconfig.h"
#include "cmdtalk.h"

using namespace std;

// Separator char used in words and tags lists.
static const string sepchars("\t");

static CmdTalk *o_talker;
static bool o_starterror{false};
static string o_cmdpath;
std::mutex o_mutex;
static string o_taggername{"Okt"};

// The Python/Java splitter is leaking memory. We restart it from time to time
static uint64_t restartcount;
static uint64_t restartthreshold = 5 * 1000 * 1000;

void TextSplit::koStaticConfInit(RclConfig *config, const string& tagger)
{
    o_cmdpath = config->findFilter("kosplitter.py");
    if (tagger == "Okt" || tagger == "Mecab") {
        o_taggername = tagger;
    } else {
        LOGERR("TextSplit::koStaticConfInit: unknown tagger [" << tagger <<
               "], using Okt\n");
    }
}

// Start the Python subprocess
static bool initCmd()
{
    if (o_starterror) {
        // No use retrying
        return false;
    }
    if (o_talker) {
        if (restartcount > restartthreshold) {
            delete o_talker;
            o_talker = nullptr;
            restartcount = 0;
        } else {
            return true;
        }
    }
    if (o_cmdpath.empty()) {
        return false;
    }
    if (nullptr == (o_talker = new CmdTalk(300))) {
        o_starterror = true;
        return false;
    }
    if (!o_talker->startCmd(o_cmdpath)) {
        delete o_talker;
        o_talker = nullptr;
        o_starterror = true;
        return false;
    }
    return true;
}

bool TextSplit::ko_to_words(Utf8Iter *itp, unsigned int *cp)
{
    std::unique_lock<std::mutex> mylock(o_mutex);
    initCmd();
    if (nullptr == o_talker) {
        return false;
    }

    LOGDEB1("k_to_words: m_wordpos " << m_wordpos << "\n");
    Utf8Iter &it = *itp;
    unsigned int c = 0;

    unordered_map<string, string> args;

    args.insert(pair<string,string>{"data", string()});
    string& inputdata{args.begin()->second};

    // We send the tagger name every time but it's only used the first
    // one: can't change it after init. We could avoid sending it
    // every time, but I don't think that the performance hit is
    // significant
    args.insert(pair<string,string>{"tagger", o_taggername});
    
    // Walk the Korean characters section and send the text to the
    // analyser
    string::size_type orgbytepos = it.getBpos();
    for (; !it.eof(); it++) {
        c = *it;
        if (!isHANGUL(c) && isalpha(c)) {
            // Done with Korean stretch, process and go back to main routine
            std::cerr << "Broke on char " << (std::string)it << endl;
            break;
        } else {
            it.appendchartostring(inputdata);
        }
    }
    LOGDEB1("TextSplit::k_to_words: sending out " << inputdata.size() <<
            " bytes " << inputdata << endl);
    restartcount += inputdata.size();
    unordered_map<string,string> result;
    if (!o_talker->talk(args, result)) {
        LOGERR("Python splitter for Korean failed\n");
        return false;
    }

    auto resit = result.find("text");
    if (resit == result.end()) {
        LOGERR("No text in Python splitter for Korean\n");
        return false;
    }        
    string& outtext = resit->second;
    vector<string> words;
    stringToTokens(outtext, words, sepchars);

    resit = result.find("tags");
    if (resit == result.end()) {
        LOGERR("No tags in Python splitter for Korean\n");
        return false;
    }        
    string& outtags = resit->second;
    vector<string> tags;
    stringToTokens(outtags, tags, sepchars);

    // This is the position in the whole text, not the local fragment,
    // which is bytepos-orgbytepos
    string::size_type bytepos(orgbytepos);
    for (unsigned int i = 0; i < words.size(); i++) {
        // The POS tagger strips characters from the input (e.g. multiple
        // spaces, sometimes new lines, possibly other stuff). This
        // means that we can't easily reconstruct the byte position
        // from the concatenated terms. The output seems to be always
        // shorter than the input, so we try to look ahead for the
        // term. Can't be too sure that this works though, depending
        // on exactly what transformation may have been applied from
        // the original input to the term.
        string word = words[i];
        trimstring(word);
        string::size_type newpos = bytepos - orgbytepos;
        newpos = inputdata.find(word, newpos);
        if (newpos != string::npos) {
            bytepos = orgbytepos + newpos;
        }
        LOGDEB1("WORD OPOS " << bytepos-orgbytepos <<
                " FOUND POS " << newpos << endl);
        if (tags[i] == "Noun" || tags[i] == "Verb" ||
            tags[i] == "Adjective" || tags[i] == "Adverb") {
            if (!takeword(
                    word, m_wordpos++, bytepos, bytepos + words[i].size())) {
                return false;
            }
        }
        LOGDEB1("WORD [" << words[i] << "] size " << words[i].size() <<
               " TAG " << tags[i] << endl);
        bytepos += words[i].size();
    }

#if DO_CHECK_THINGS
    int sizediff = inputdata.size() - (bytepos - orgbytepos);
    if (sizediff < 0)
        sizediff = -sizediff;
    if (sizediff > 1) {
        LOGERR("ORIGINAL TEXT SIZE: " << inputdata.size() <<
               " FINAL BYTE POS " << bytepos - orgbytepos <<
               " TEXT [" << inputdata << "]\n");
    }
#endif
    
    // Reset state, saving term position, and return the found non-cjk
    // Unicode character value. The current input byte offset is kept
    // in the utf8Iter
    int pos = m_wordpos;
    clearsplitstate();
    m_spanpos = m_wordpos = pos;
    *cp = c;
    return true;
}
