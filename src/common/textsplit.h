/*
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _TEXTSPLIT_H_INCLUDED_
#define _TEXTSPLIT_H_INCLUDED_
/* @(#$Id: textsplit.h,v 1.14 2006-11-20 11:17:53 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#ifndef NO_NAMESPACES
using std::string;
#endif

/**
 * Function class whose takeword method is called for every detected word while * splitting text.
 */
class TextSplitCB {
 public:
    virtual ~TextSplitCB() {}
    virtual bool takeword(const std::string& term, 
			  int pos,  // term pos
			  int bts,  // byte offset of first char in term
			  int bte   // byte offset of first char after term
			  ) = 0; 
};

/** 
 * Split text into words. 
 * See comments at top of .cpp for more explanations.
 * This uses a callback function. It could be done with an iterator instead,
 * but 'ts much simpler this way...
 */
class TextSplit {
 public:
    enum Flags {TXTS_NONE = 0, TXTS_ONLYSPANS = 1, TXTS_NOSPANS = 2};
    /**
     * Constructor: just store callback object
     */
    TextSplit(TextSplitCB *t, Flags flags = TXTS_NONE) 
	: m_flags(flags), cb(t), maxWordLength(40), prevpos(-1) {}
    /**
     * Split text, emit words and positions.
     */
    bool text_to_words(const std::string &in);

 private:
    Flags m_flags;
    TextSplitCB *cb;
    int maxWordLength;

    string span; // Current span. Might be jf.dockes@wanadoo.f
    int wordStart; // Current word: no punctuation at all in there
    unsigned int wordLen;
    bool number;
    int wordpos; // Term position of current word
    int spanpos; // Term position of current span

    // It may happen that our cleanup would result in emitting the
    // same term twice. We try to avoid this
    int prevpos;
    unsigned int prevlen;

    bool emitterm(bool isspan, std::string &term, int pos, int bs, int be);
    bool doemit(bool spanerase, int bp);
};

#endif /* _TEXTSPLIT_H_INCLUDED_ */
