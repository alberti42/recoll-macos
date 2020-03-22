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

static CmdTalk *o_talker;
static bool o_starterror{false};
static string o_cmdpath;
std::mutex o_mutex;

void TextSplit::koStaticConfInit(RclConfig *config)
{
    o_cmdpath = config->findFilter("kosplitter.py");
}

static bool initCmd()
{
    if (o_starterror) {
        // No use retrying
        return false;
    }
    if (o_talker) {
        return true;
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
    if (nullptr == o_talker) {
        if (!initCmd()) {
            return false;
        }
    }
    LOGDEB1("k_to_words: m_wordpos " << m_wordpos << "\n");
    Utf8Iter &it = *itp;
    unsigned int c = 0;
    unordered_map<string, string> args;
    args.insert(pair<string,string>{"data", string()});
    string& inputdata{args.begin()->second};
    
    // Gather all Korean characters and send the text to the analyser
    for (; !it.eof(); it++) {
        c = *it;
        if (!isHANGUL(c) && !(isascii(c) && (isspace(c) || ispunct(c)))) {
            // Done with Korean stretch, process and go back to main routine
            //std::cerr << "Broke on char " << int(c) << endl;
            break;
        } else {
            it.appendchartostring(inputdata);
        }
    }
    LOGDEB1("TextSplit::k_to_words: sending out " << inputdata.size() <<
            " bytes " << inputdata << endl);
    unordered_map<string,string> result;
    if (!o_talker->talk(args, result)) {
        LOGERR("Python splitter for Korean failed\n");
        return false;
    }
    auto resit = result.find("data");
    if (resit == result.end()) {
        LOGERR("No data in Python splitter for Korean\n");
        return false;
    }        
    string& outdata = resit->second;
    char sepchar = '^';
    //std::cerr << "GOT FROM SPLITTER: " << outdata << endl;
    string::size_type wordstart = 0;
    string::size_type wordend = outdata.find(sepchar);
    for (;;) {
        //cerr << "start " << wordstart << " end " << wordend << endl;        
        if (wordend != wordstart) {
            string::size_type len = (wordend == string::npos) ?
                wordend : wordend-wordstart;
            string word = outdata.substr(wordstart, len);
            //cerr << " WORD[" <<  word << "]\n";
            if (!takeword(word, m_wordpos++, 0, 0)) {
                return false;
            }
        }
        if (wordend == string::npos)
            break;
        wordstart = wordend + 1;
        wordend = outdata.find(sepchar, wordstart);
    }
    

    // Reset state, saving term position, and return the found non-cjk
    // Unicode character value. The current input byte offset is kept
    // in the utf8Iter
    int pos = m_wordpos;
    clearsplitstate();
    m_spanpos = m_wordpos = pos;
    *cp = c;
    return true;
}
