#ifndef lint
static char	rcsid[] = "@(#$Id: csguess.cpp,v 1.1 2004-12-15 08:21:05 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
// This code was converted from estraier / qdbm / myconf.c
#include <errno.h>

#include <iconv.h>
#include "csguess.h"

#include <string>
using std::string;

// The values from estraier were 32768, 256, 0.001
const int ICONVCHECKSIZ = 4000;
const int ICONVMISSMAX  = 10;
const double ICONVALLWRAT = 0.001;

// Try to transcode and count errors (for charset guessing)
static int transcodeErrCnt(const char *ptr, int size, 
			   const char *icode, const char *ocode)
{
    iconv_t ic;
    char obuf[ICONVCHECKSIZ], *wp, *rp;
    size_t isiz, osiz;
    int miss;
    isiz = size;
    if((ic = iconv_open(ocode, icode)) == (iconv_t)-1) return ICONVMISSMAX;
    miss = 0;
    rp = (char *)ptr;
    while(isiz > 0){
	osiz = ICONVCHECKSIZ;
	wp = obuf;
	if(iconv(ic, (const char **)&rp, &isiz, &wp, &osiz) == -1){
	    if(errno == EILSEQ || errno == EINVAL){
		rp++;
		isiz--;
		miss++;
		if(miss >= ICONVMISSMAX) 
		    break;
	    } else {
		break;
	    }
	}
    }
    if(iconv_close(ic) == -1) 
	return ICONVMISSMAX;
    return miss;
}


string csguess(const string &in)
{
    const char     *hypo;
    int		i, miss;
    const char *text = in.c_str();
    bool cr = false;

    int size = in.length();
    if (size > ICONVCHECKSIZ)
	size = ICONVCHECKSIZ;

    // UTF-16 with normal prefix ?
    if (size >= 2 && (!memcmp(text, "\xfe\xff", 2) || 
		      !memcmp(text, "\xff\xfe", 2)))
	return "UTF-16";

    // If we find a zero at an appropriate position, guess it's UTF-16 
    // anyway. This is a quite expensive test for other texts as we'll 
    // have to scan the whole thing.
    for (i = 0; i < size - 1; i += 2) {
	if (text[i] == 0 && text[i + 1] != 0)
	    return "UTF-16BE";
	if (text[i + 1] == 0 && text[i] != 0)
	    return "UTF-16LE";
    }

    // Look for iso-2022 specific escape sequences. As iso-2022 begins
    // in ascii, these succeed fast for a japanese text, but are quite
    // expensive for any other
    for (i = 0; i < size - 3; i++) {
	if (text[i] == 0x1b) {
	    i++;
	    if (text[i] == '(' && strchr("BJHI", text[i + 1]))
		return "ISO-2022-JP";
	    if (text[i] == '$' && strchr("@B(", text[i + 1]))
		return "ISO-2022-JP";
	}
    }

    // Try conversions from ascii and utf-8. These are unlikely to succeed
    // by mistake.
    if (transcodeErrCnt(text, size, "US-ASCII", "UTF-16BE") < 1)
	return "US-ASCII";

    if (transcodeErrCnt(text, size, "UTF-8", "UTF-16BE") < 1)
	return "UTF-8";

    hypo = 0;
    for (i = 0; i < size; i++) {
	if (text[i] == 0xd) {
	    cr = true;
	    break;
	}
    }

    if (cr) {
	if ((miss = transcodeErrCnt(text, size, "Shift_JIS", "EUC-JP")) < 1)
	    return "Shift_JIS";
	if (!hypo && miss / (double)size <= ICONVALLWRAT)
	    hypo = "Shift_JIS";
	if ((miss = transcodeErrCnt(text, size, "EUC-JP", "UTF-16BE")) < 1)
	    return "EUC-JP";
	if (!hypo && miss / (double)size <= ICONVALLWRAT)
	    hypo = "EUC-JP";
    } else {
	if ((miss = transcodeErrCnt(text, size, "EUC-JP", "UTF-16BE")) < 1)
	    return "EUC-JP";
	if (!hypo && miss / (double)size <= ICONVALLWRAT)
	    hypo = "EUC-JP";
	if ((miss = transcodeErrCnt(text, size, "Shift_JIS", "EUC-JP")) < 1)
	    return "Shift_JIS";
	if (!hypo && miss / (double)size <= ICONVALLWRAT)
	    hypo = "Shift_JIS";
    }
    if ((miss = transcodeErrCnt(text, size, "UTF-8", "UTF-16BE")) < 1)
	return "UTF-8";
    if (!hypo && miss / (double)size <= ICONVALLWRAT)
	hypo = "UTF-8";
    if ((miss = transcodeErrCnt(text, size, "CP932", "UTF-16BE")) < 1)
	return "CP932";
    if (!hypo && miss / (double)size <= ICONVALLWRAT)
	hypo = "CP932";

    return hypo ? hypo : "ISO-8859-1";
}
