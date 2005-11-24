#ifndef lint
static char rcsid[] = "@(#$Id: base64.cpp,v 1.3 2005-11-24 07:16:16 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <sys/types.h>

#include <string>
#ifndef NO_NAMESPACES
using std::string;
#endif /* NO_NAMESPACES */

//#define DEBUG_BASE64 
#ifdef DEBUG_BASE64
#define DPRINT(X) fprintf X
#else
#define DPRINT(X)
#endif

// This is adapted from FreeBSD's code.
static const char Base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';
bool base64_decode(const string& in, string& out)
{
    int io = 0, state = 0, ch;
    char *pos;
    unsigned int ii = 0;
    out.reserve(in.length());

    for (ii = 0; ii < in.length(); ii++) {
	ch = in[ii];
	if (isspace((unsigned char)ch))        /* Skip whitespace anywhere. */
	    continue;

	if (ch == Pad64)
	    break;

	pos = strchr(Base64, ch);
	if (pos == 0) {
	    /* A non-base64 character. */
	    DPRINT((stderr, "base64_dec: non-base64 char at pos %d\n", ii));
	    return false;
	}


	switch (state) {
	case 0:
	    out += (pos - Base64) << 2;
	    state = 1;
	    break;
	case 1:
	    out[io]   |=  (pos - Base64) >> 4;
	    out += ((pos - Base64) & 0x0f) << 4 ;
	    io++;
	    state = 2;
	    break;
	case 2:
	    out[io]   |=  (pos - Base64) >> 2;
	    out += ((pos - Base64) & 0x03) << 6;
	    io++;
	    state = 3;
	    break;
	case 3:
	    out[io] |= (pos - Base64);
	    io++;
	    state = 0;
	    break;
	default:
	    DPRINT((stderr, "base64_dec: internal!bad state!\n"));
	    return false;
	}
    }

    /*
     * We are done decoding Base-64 chars.  Let's see if we ended
     * on a byte boundary, and/or with erroneous trailing characters.
     */

    if (ch == Pad64) {		/* We got a pad char. */
	ch = in[ii++];		/* Skip it, get next. */
	switch (state) {
	case 0:		/* Invalid = in first position */
	case 1:		/* Invalid = in second position */
	    DPRINT((stderr, "base64_dec: pad char in state 0/1\n"));
	    return false;

	case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
	    for (; ii < in.length(); ch = in[ii++])
		if (!isspace((unsigned char)ch))
		    break;
	    /* Make sure there is another trailing = sign. */
	    if (ch != Pad64) {
		DPRINT((stderr, "base64_dec: missing pad char!\n"));
		// Well, there are bad encoders out there. Let it pass
		// return false;
	    }
	    ch = in[ii++];		/* Skip the = */
	    /* Fall through to "single trailing =" case. */
	    /* FALLTHROUGH */

	case 3:	    /* Valid, means two bytes of info */
	    /*
	     * We know this char is an =.  Is there anything but
	     * whitespace after it?
	     */
	    for (; ii < in.length(); ch = in[ii++])
		if (!isspace((unsigned char)ch)) {
		    DPRINT((stderr, "base64_dec: non-white at eod: 0x%x\n", 
			    (unsigned int)ch));
		    // Well, there are bad encoders out there. Let it pass
		    //return false;
		}

	    /*
	     * Now make sure for cases 2 and 3 that the "extra"
	     * bits that slopped past the last full byte were
	     * zeros.  If we don't check them, they become a
	     * subliminal channel.
	     */
	    if (out[io] != 0) {
		DPRINT((stderr, "base64_dec: bad extra bits!\n"));
		// Well, there are bad encoders out there. Let it pass
		out[io] = 0;
		// return false;
	    }
	}
    } else {
	/*
	 * We ended by seeing the end of the string.  Make sure we
	 * have no partial bytes lying around.
	 */
	if (state != 0) {
	    DPRINT((stderr, "base64_dec: bad final state\n"));
	    return false;
	}
    }

    DPRINT((stderr, "base64_dec: ret ok, io %d sz %d len %d value [%s]\n", 
	    io, out.size(), out.length(), out.c_str()));
    return true;
}

#undef Assert
#define Assert(X)

void base64_encode(const string &in, string &out)
{
    size_t datalength = 0;
    unsigned char input[3];
    unsigned char output[4];
    size_t i;

    int srclength = in.length();
    int sidx = 0;
    while (2 < srclength) {
	input[0] = in[sidx++];
	input[1] = in[sidx++];
	input[2] = in[sidx++];
	srclength -= 3;

	output[0] = input[0] >> 2;
	output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
	output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
	output[3] = input[2] & 0x3f;
	Assert(output[0] < 64);
	Assert(output[1] < 64);
	Assert(output[2] < 64);
	Assert(output[3] < 64);

	out += Base64[output[0]];
	out += Base64[output[1]];
	out += Base64[output[2]];
	out += Base64[output[3]];
    }
    
    /* Now we worry about padding. */
    if (0 != srclength) {
	/* Get what's left. */
	input[0] = input[1] = input[2] = '\0';
	for (int i = 0; i < srclength; i++)
	    input[i] = in[sidx++];
	
	output[0] = input[0] >> 2;
	output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
	output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
	Assert(output[0] < 64);
	Assert(output[1] < 64);
	Assert(output[2] < 64);

	out += Base64[output[0]];
	out += Base64[output[1]];
	if (srclength == 1)
	    out += Pad64;
	else
	    out += Base64[output[2]];
	out += Pad64;
    }
    return;
}

#ifdef TEST_BASE64
#include <stdio.h>
int main(int agrc, char **argv)
{
    string in = "12345";
    string out;
    base64_encode(in, out);
    printf("in %s out %s\n", in.c_str(), out.c_str());
}
#endif
