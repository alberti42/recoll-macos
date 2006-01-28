#ifndef _TEXTSPLIT_H_INCLUDED_
#define _TEXTSPLIT_H_INCLUDED_
/* @(#$Id: textsplit.h,v 1.9 2006-01-28 10:23:55 dockes Exp $  (C) 2004 J.F.Dockes */

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
    bool fq;        // for query:  Are we splitting for query or index ?
    // It may happen that our cleanup would result in emitting the
    // same term twice. We try to avoid this
    string prevterm;
    int prevpos;
    TextSplitCB *cb;
    int maxWordLength;
    bool emitterm(bool isspan, std::string &term, int pos, int bs, int be);
    bool doemit(string &word, int &wordpos, string &span, int &spanpos,
		bool spanerase, int bp);
 public:
    /**
     * Constructor: just store callback object
     */
    TextSplit(TextSplitCB *t, bool forquery = false) 
	: fq(forquery), prevpos(-1), cb(t), maxWordLength(40) {}
    /**
     * Split text, emit words and positions.
     */
    bool text_to_words(const std::string &in);
};

#endif /* _TEXTSPLIT_H_INCLUDED_ */
