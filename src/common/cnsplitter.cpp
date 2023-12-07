/* Copyright (C) 2023 J.F.Dockes
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

// Specialized Chinese text splitter using Jieba running in a Python subprocess.
// https://github.com/fxsjy/jieba/

#include "autoconfig.h"

#include "cnsplitter.h"

#include <iostream>
#include <string>
#include <cstring>
#include <unordered_set>
#include <mutex>
#include <algorithm>

#include "textsplit.h"
#define LOGGER_LOCAL_LOGINC 3
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
static vector<string> o_cmdargs;
static std::mutex o_mutex;
static string o_taggername{"Jieba"};

// The Python/Java splitter may be leaking memory. We restart it from time to time
static uint64_t restartcount;
static uint64_t restartthreshold = 5 * 1000 * 1000;

void cnStaticConfInit(RclConfig *config, const string& tagger)
{
    LOGDEB0("cnStaticConfInit\n")
    std::vector<std::string> cmdvec;
    if (config->pythonCmd("cnsplitter.py", cmdvec)) {
        auto it = cmdvec.begin();
        o_cmdpath = *it++;
        o_cmdargs.clear();
        o_cmdargs.insert(o_cmdargs.end(), it, cmdvec.end());
    } else {
        LOGERR("cnStaticConfInit: cnsplitter.py Python script not found.\n");
        o_starterror = true;
        return;
    }
    o_taggername = tagger;
    LOGDEB0("cnStaticConfInit: tagger name " << tagger << " cmd " << o_cmdpath << " args " <<
            stringsToString(o_cmdargs) << "\n")
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
    if (nullptr == (o_talker = new CmdTalk(300))) {
        o_starterror = true;
        return false;
    }
    if (!o_talker->startCmd(o_cmdpath, o_cmdargs)) {
        delete o_talker;
        o_talker = nullptr;
        o_starterror = true;
        return false;
    }
    return true;
}

#define LOGCN LOGDEB

using StrSz = std::string::size_type;
using WordChars = std::tuple<std::string,int,int>;

// 160 is nbsp
#define ISASCIIPUNCTORCTL(c) ((c <= 0x7f || c == 160) &&               \
                              ! ((c >= 'A' && c <= 'Z') ||             \
                                 (c >= 'a' && c <= 'z') ||             \
                                 (c >= '0' && c <= '9')))

bool CNSplitter::text_to_words(Utf8Iter& it, unsigned int *cp, int& wordpos)
{
    LOGDEB1("CNSplitter::text_to_words\n");
    int flags = m_sink.flags();
    
    std::unique_lock<std::mutex> mylock(o_mutex);
    initCmd();
    if (nullptr == o_talker) {
        return false;
    }

    LOGDEB1("cn_to_words: wordpos " << wordpos << "\n");
    unsigned int c = 0;

    unordered_map<string, string> args;

    args.insert(pair<string,string>{"data", string()});
    string& inputdata(args.begin()->second);

    // We send the tagger name every time but it's only used the first
    // one: can't change it after init. We could avoid sending it
    // every time, but I don't think that the performance hit is
    // significant
    args.insert(pair<string,string>{"tagger", o_taggername});
    
    // Walk the Chinese characters section, and accumulate tagger input.
    // While doing this, we compute spans (space-less chunks), which
    // we will index in addition to the parts.
    // We also strip some useless chars, and prepare page number computations.
    StrSz orgbytepos = it.getBpos();
    // We keep a char offset to byte offset translation
    std::vector<int> chartobyte;
    std::vector<int> pagebreaks;
    for (; !it.eof() && !it.error(); it++) {
        c = *it;
        chartobyte.push_back(it.getBpos());

        if (!TextSplit::isCHINESE(c) && !ISASCIIPUNCTORCTL(c)) {
            // Non-Chinese: we keep on if encountering space and other ASCII punctuation. Allows
            // sending longer pieces of text to the splitter (perf). Else break, process this piece,
            // and return to the main splitter
//            LOGCN("cn_to_words: broke on [" << (std::string)it << "] code " << c << "\n");
            break;
        } else {
            if (c == '\f') {
                inputdata += ' ';
                pagebreaks.push_back(it.getBpos());
            } else {
                if (ISASCIIPUNCTORCTL(c)) {
                    inputdata += ' ';
                } else {
                    it.appendchartostring(inputdata);
                }
            }
        }
    }
        
// LOGCN("TextSplit::cn_to_words: sending out " << inputdata.size() << " bytes " << inputdata << "\n");

    // Overall data counter for worker restarts
    restartcount += inputdata.size();
    // Have the worker analyse the data, check that we get a result,
    unordered_map<string,string> result;
    if (!o_talker->talk(args, result)) {
        LOGERR("Python splitter for Chinese failed for [" << inputdata << "]\n");
        return false;
    }

    // Split the resulting words and positions strings into vectors. This could be optimized for
    // less data copying...

    auto resit = result.find("text");
    if (resit == result.end()) {
        LOGERR("No text found in Python splitter for Chinese output\n");
        return false;
    }        
    string& outtext = resit->second;
    vector<string> words;
    stringToTokens(outtext, words, sepchars);

    resit = result.find("charoffsets");
    if (resit == result.end()) {
        LOGERR("No positions in Python splitter for Chinese output\n");
        return false;
    }        
    string& spos = resit->second;
    vector<string> scharoffsets;
    stringToTokens(spos, scharoffsets, sepchars);
    if (scharoffsets.size() != words.size() * 2) {
        LOGERR("CNSplitter: words sz " << words.size() << " offsets sz " <<scharoffsets.size() << "\n");
        return false;
    }
    LOGINF("CNSplitter got back " << words.size() << " words\n");
    
    vector<int> charoffsets;
    charoffsets.reserve(scharoffsets.size());
    for (const auto& s: scharoffsets) {
        charoffsets.push_back(atoi(s.c_str()));
    }

    std::vector<WordChars> wordchars;
    for (int i = 0; i < int(words.size()); i++) {
        auto w = words[i];
        trimstring(w);
        if (w.empty())
            continue;
        wordchars.push_back(std::tuple<std::string,int,int>(w, charoffsets[i*2], charoffsets[i*2+1]));
    }
    // The splitter sends words and spans. Span come after the words, but we prefer spans first
    std::sort(wordchars.begin(), wordchars.end(), [](WordChars& a, WordChars& b) {
        return std::get<1>(a) < std::get<1>(b) || ((std::get<1>(a) == std::get<1>(b)) &&
                                                   (std::get<2>(a) > std::get<2>(b)));
    });
    for (const auto& wc : wordchars) {
        auto w = std::get<0>(wc);
        std::cerr << w << "\t" << std::get<1>(wc) << " " << std::get<2>(wc) << "\n";
        if (w.size() == 1) {
            std::cerr << "CHARACTER " << int(w[0]) << "\n";
        }
    }
    // Process the words and positions
    // Current span
    int spanstart{0}, spanend{0};
    for (const auto& wc : wordchars) {
        if (std::get<2>(wc) > spanend) {
            // This is either a standalone word or a span
            if (std::get<1>(wc) != spanstart)
                wordpos++;
            spanstart = std::get<1>(wc);
            spanend = std::get<2>(wc);
            std::cerr << std::get<0>(wc) << " AT POS " << wordpos << "\n";
            if (!m_sink.takeword(std::get<0>(wc), wordpos, orgbytepos + chartobyte[spanstart],
                                 orgbytepos + chartobyte[spanend])) {
                return false;
            }
        } else {
            // This word is covered by a span
            if (std::get<1>(wc) != spanstart)
                wordpos++;
            std::cerr << std::get<0>(wc) << " AT POS " << wordpos << "\n";
            if (!(flags & TextSplit::TXTS_ONLYSPANS) &&
                !m_sink.takeword(std::get<0>(wc), wordpos, chartobyte[std::get<1>(wc)],
                                 chartobyte[std::get<2>(wc)])) {
                return false;
            }
        }
    }

    // Return the found non-chinese Unicode character value. The current input byte offset is kept
    // in the utf8Iter
    *cp = c;
    LOGDEB1("cn_to_words: returning\n");
    return true;
}
