#ifndef _TEXTSPLIT_H_INCLUDED_
#define _TEXTSPLIT_H_INCLUDED_
/* @(#$Id: textsplit.h,v 1.1 2004-12-14 17:49:11 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

/** 
 * Split text into words. 
 * See comments at top of .cpp for more explanations.
 * This used a callback function. It could be done with an iterator instead,
 * but 'ts much simpler this way...
 */
class TextSplit {
 public:
    typedef int (*TermSink)(void *cdata, const std::string & term, int pos);
 private:
    TermSink termsink;
    void *cdata;
    void emitterm(std::string &term, int pos, bool doerase);
 public:
    /**
     * Constructor: just store callback and client data
     */
    TextSplit(TermSink t, void *c) : termsink(t), cdata(c) {}
    /**
     * Split text, emit words and positions.
     */
    void text_to_words(const std::string &in);
};

#endif /* _TEXTSPLIT_H_INCLUDED_ */
