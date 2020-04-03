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
static bool isKomoran{false};

// The Python/Java splitter is leaking memory. We restart it from time to time
static uint64_t restartcount;
static uint64_t restartthreshold = 5 * 1000 * 1000;

static const string magicpage{"NEWPPPAGE"};

void TextSplit::koStaticConfInit(RclConfig *config, const string& tagger)
{
    o_cmdpath = config->findFilter("kosplitter.py");
    if (tagger == "Okt" || tagger == "Mecab" || tagger == "Komoran") {
        o_taggername = tagger;
        if (tagger == "Komoran")
            isKomoran = true;
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
    LOGDEB1("ko_to_words\n");
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
    for (; !it.eof() && !it.error(); it++) {
        c = *it;
        if (!isHANGUL(c) && isalpha(c)) {
            // Done with Korean stretch, process and go back to main routine
            LOGDEB1("ko_to_words: broke on " << (std::string)it << endl);
            break;
        } else {
            if (c == '\f') {
                inputdata += magicpage + " ";
            } else {
                if (c < 0x20 || (c > 0x7e && c < 0xa0)) {
                    inputdata += ' ';
                } else {
                    it.appendchartostring(inputdata);
                }
            }
        }
    }
    LOGDEB1("TextSplit::k_to_words: sending out " << inputdata.size() <<
            " bytes " << inputdata << endl);
    restartcount += inputdata.size();
    unordered_map<string,string> result;
    if (!o_talker->talk(args, result)) {
        LOGERR("Python splitter for Korean failed for [" << inputdata << "]\n");
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

    // This is the position in the local fragment,
    // not in the whole text which is orgbytepos + bytepos
    string::size_type bytepos{0};
    string::size_type pagefix{0};
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
        if (word == magicpage) {
            LOGDEB1("ko_to_words: NEWPAGE\n");
            newpage(m_wordpos);
            bytepos += word.size() + 1;
            pagefix += word.size();
            continue;
        }
        // Find the actual start position of the word in the section.
        string::size_type newpos = inputdata.find(word, bytepos);
        if (newpos != string::npos) {
            bytepos = newpos;
        } else {
            LOGDEB("textsplitko: word [" << word << "] not found in text\n");
        }
        LOGDEB1("WORD [" << word << "] size " << word.size() <<
                " TAG " << tags[i] << " inputdata size " << inputdata.size() <<
                " absbytepos " << orgbytepos + bytepos << 
                " bytepos " << bytepos << " word from text: " <<
                inputdata.substr(bytepos, word.size()) << endl);
        if (tags[i] == "Noun" || tags[i] == "Verb" ||
            tags[i] == "Adjective" || tags[i] == "Adverb") {
            string::size_type abspos = orgbytepos + bytepos - pagefix;
            if (!takeword(word, m_wordpos++, abspos, abspos + word.size())) {
                return false;
            }
        }
        bytepos += word.size();
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
    LOGDEB1("ko_to_words: returning\n");
    return true;
}
