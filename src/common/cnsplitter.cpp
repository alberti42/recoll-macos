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
#include <mutex>
#include <algorithm>

#include "textsplit.h"
//#define LOGGER_LOCAL_LOGINC 3
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

// The external splitter process may be leaking memory. We restart it from time to time. It costs a
// couple seconds every time.
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

#define LOGCN LOGDEB0

using StrSz = std::string::size_type;

struct WordAndPos {
    WordAndPos(const std::string& w, int s, int e)
        : word(w), startpos(s), endpos(e) {}
    std::string word;
    int startpos;
    int endpos;
};

// Characters which will trigger a return to the caller: don't do it for ascii punctuation or
// control chars: only do it for ASCII alphanum or non chinese unicode. 160 is nbsp.
#define ISASCIIPUNCTORCTL(c) ((c <= 0x7f || c == 160) &&               \
                              ! ((c >= 'A' && c <= 'Z') ||             \
                                 (c >= 'a' && c <= 'z') ||             \
                                 (c >= '0' && c <= '9')))

bool CNSplitter::text_to_words(Utf8Iter& it, unsigned int *cp, int& wordpos)
{
    LOGDEB0("CNSplitter::text_to_words: wordpos " << wordpos << "\n");
    std::unique_lock<std::mutex> mylock(o_mutex);

    int flags = m_sink.flags();
    initCmd();
    if (nullptr == o_talker) {
        return false;
    }

    unsigned int c = 0;

    // Args for the cmdtalk input to our subprocess. 
    unordered_map<string, string> args;
    // Input data. We use a reference to the string to avoid a copy
    args.insert(pair<string,string>{"data", string()});
    string& inputdata(args.begin()->second);
    // We send the tagger name every time but it's only used the first
    // one: can't change it after init. We could avoid sending it
    // every time, but I don't think that the performance hit is
    // significant
    args.insert(pair<string,string>{"tagger", o_taggername});
    

    // We keep a Unicode character offset to byte offset translation
    std::vector<int> chartobyte;
    // And we record the page breaks (TBD use it !)
    std::vector<int> pagebreaks;

    // Walk the Chinese characters section, and accumulate tagger input.
    for (; !it.eof() && !it.error(); it++) {
        c = *it;

        if (!TextSplit::isCHINESE(c) && !ISASCIIPUNCTORCTL(c)) {
            // Non-Chinese: we keep on if encountering space and other ASCII punctuation. Allows
            // sending longer pieces of text to the splitter (perf). Else break, process this piece,
            // and return to the main splitter
            //LOGCN("cn_to_words: broke on [" << (std::string)it << "] code " << c << "\n");
            break;
        } else {
            if (c == '\f') {
                inputdata += ' ';
                pagebreaks.push_back(chartobyte.size());
            } else {
                if (ISASCIIPUNCTORCTL(c)) {
                    inputdata += ' ';
                } else {
                    if (TextSplit::whatcc(c) == TextSplit::SPACE) {
                        // Avoid Jieba returning tokens for punctuation. Easier to remove here than
                        // later
                        inputdata += ' ';
                    } else {
                        it.appendchartostring(inputdata);
                    }
                }
            }
            chartobyte.push_back(it.getBpos());
        }
    }
    chartobyte.push_back(it.getBpos());
        
    LOGCN("CNSplitter::text_to_words: send " << inputdata.size() << " bytes " << inputdata << "\n");

    // Overall data counter for worker restarts
    restartcount += inputdata.size();
    // Have the worker analyse the data, check that we get a result,
    unordered_map<string,string> result;
    if (!o_talker->talk(args, result)) {
        LOGERR("Python splitter for Chinese failed for [" << inputdata << "]\n");
        return false;
    }

    // Split the resulting words and positions string into vectors of structs.
    auto resit = result.find("wordsandpos");
    if (resit == result.end()) {
        LOGERR("No values found in output from Python splitter for Chinese.\n");
        return false;
    }        
    char *data = const_cast<char *>(resit->second.c_str());
    vector<WordAndPos> words;
    char *saveptr{nullptr};
    for (;;) {
        auto w = strtok_r(data, "\t", &saveptr);
        data = nullptr;
        if (nullptr == w) {
            break;
        }
        auto s = strtok_r(nullptr, "\t", &saveptr);
        if (nullptr == s) {
            break;
        }
        auto e = strtok_r(nullptr, "\t", &saveptr);
        if (nullptr == e) {
            break;
        }
        std::string word(w);
        trimstring(word);
        if (word.empty())
            continue;
        words.emplace_back(w, atoi(s), atoi(e));
    }
    // The splitter sends words and spans. Jieba emits spans after the words they cover, but we
    // prefer spans first
    std::sort(words.begin(), words.end(), [](WordAndPos& a, WordAndPos& b) {
        return a.startpos < b.startpos || ((a.startpos == b.startpos) && (a.endpos > b.endpos));
    });

    // Process the words and positions
    int wstart{0}, spanend{0};
    unsigned int pagebreakidx{0};
    for (const auto& wc : words) {
        LOGCN(wc.word << " " << wc.startpos << " " << wc.endpos << "\n");
        // startpos is monotonic because it's the 1st sort key.
        if (wc.startpos > wstart) {
            wstart = wc.startpos;
            wordpos++;
        }
        if (pagebreakidx < pagebreaks.size() && wc.startpos > pagebreaks[pagebreakidx]) {
            LOGERR("NEWPAGE\n");
            m_sink.newpage(wordpos);
            pagebreakidx++;
        }
        if (wc.endpos > spanend) {
            spanend = wc.endpos;
            LOGCN("SPAN " << wc.word << " AT POS " << wordpos << "\n");
            if (!m_sink.takeword(wc.word, wordpos, chartobyte[wc.startpos], chartobyte[wc.endpos])) {
                return false;
            }
        } else {
            // This word is covered by a span
            LOGCN("WORD " << wc.word << " AT POS " << wordpos << "\n");
            if (!(flags & TextSplit::TXTS_ONLYSPANS) &&
                !m_sink.takeword(wc.word, wordpos, chartobyte[wc.startpos], chartobyte[wc.endpos])) {
                return false;
            }
        }
    }
    wordpos++;
    
    // Return the found non-chinese Unicode character value. The current input byte offset is kept
    // in the utf8Iter
    *cp = c;
    LOGDEB1("cn_to_words: returning\n");
    return true;
}
