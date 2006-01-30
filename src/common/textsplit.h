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
/* @(#$Id: textsplit.h,v 1.11 2006-01-30 11:15:27 dockes Exp $  (C) 2004 J.F.Dockes */

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
    /**
     * Constructor: just store callback object
     */
    TextSplit(TextSplitCB *t, bool forquery = false) 
	: fq(forquery), cb(t), maxWordLength(40), prevpos(-1) {}
    /**
     * Split text, emit words and positions.
     */
    bool text_to_words(const std::string &in);

 private:
    bool fq;        // for query:  Are we splitting for query or index ?
    TextSplitCB *cb;
    int maxWordLength;

    string span; // Current span. Might be jf.dockes@wanadoo.f
    string word; // Current word: no punctuation at all in there
    bool number;
    int wordpos; // Term position of current word
    int spanpos; // Term position of current span
    int charpos; // Character position

    // It may happen that our cleanup would result in emitting the
    // same term twice. We try to avoid this
    int prevpos;
    string prevterm;

    bool emitterm(bool isspan, std::string &term, int pos, int bs, int be);
    bool doemit(bool spanerase, int bp);
};

#endif /* _TEXTSPLIT_H_INCLUDED_ */
