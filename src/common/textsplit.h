#ifndef _TEXTSPLIT_H_INCLUDED_
#define _TEXTSPLIT_H_INCLUDED_
/* @(#$Id: textsplit.h,v 1.4 2005-02-07 13:17:47 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

// Function class whose called for every detected word
class TextSplitCB {
 public:
    virtual ~TextSplitCB() {}
    virtual bool takeword(const std::string& term, 
			  int pos,  // term pos
			  int bts,      // byte offset of first char in term
			  int bte      // byte offset of first char after term
			  ) = 0; 
};

/** 
 * Split text into words. 
 * See comments at top of .cpp for more explanations.
 * This uses a callback function. It could be done with an iterator instead,
 * but 'ts much simpler this way...
 */
class TextSplit {
    TextSplitCB *cb;
    int maxWordLength;
    bool emitterm(std::string &term, int pos, bool doerase, int, int);
 public:
    /**
     * Constructor: just store callback and client data
     */
    TextSplit(TextSplitCB *t) : cb(t), maxWordLength(40) {}
    /**
     * Split text, emit words and positions.
     */
    bool text_to_words(const std::string &in);
};

#endif /* _TEXTSPLIT_H_INCLUDED_ */
