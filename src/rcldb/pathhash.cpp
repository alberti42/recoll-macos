#ifndef lint
static char rcsid[] = "@(#$Id: pathhash.cpp,v 1.4 2006-11-15 14:57:53 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
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

#include <stdio.h>

#include "pathhash.h"
#include "md5.h"
#include "base64.h"

#ifndef NO_NAMESPACES
using std::string;
namespace Rcl {
#endif /* NO_NAMESPACES */

#ifdef PATHHASH_HEX
static void md5hexprint(const unsigned char hash[16], string &out)
{
    out.erase();
    out.reserve(33);
    static const char hex[]="0123456789abcdef";
    for (int i = 0; i < 16; i++) {
	out.append(1, hex[hash[i] >> 4]);
	out.append(1, hex[hash[i] & 0x0f]);
    }
}
#endif

// Size of the hashed result (base64 of 16 bytes of md5, minus 2 pad chars)
#define HASHLEN 22

// Convert longish paths by truncating and appending hash of path
// The full length of the base64-encoded (minus pad) of the md5 is 22 chars
// We append this to the truncated path
void pathHash(const std::string &path, std::string &phash, unsigned int maxlen)
{
    if (maxlen < HASHLEN) {
	fprintf(stderr, "pathHash: internal error: requested len too small\n");
	abort();
    }

    if (path.length() <= maxlen) {
	phash = path;
	return;
    }

    // Compute the md5
    unsigned char chash[16];
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (const unsigned char *)(path.c_str()+maxlen-HASHLEN), 
	      path.length() - (maxlen - HASHLEN));
    MD5Final(chash, &ctx);

#if 0
    string hex;
    md5hexprint(chash, hex);
    printf("hex  [%s]\n", hex.c_str());
#endif

    // Encode it to ascii. This shouldn't be strictly necessary as
    // xapian terms can be binary
    string hash;
    base64_encode(string((char *)chash, 16), hash);
    // We happen to know there will be 2 pad chars in there, that we
    // don't need as this won't ever be decoded. Resulting length is 22
    hash.resize(hash.length() - 2);


    // Truncate path and append hash
    phash = path.substr(0, maxlen - HASHLEN) + hash;
}
#ifndef NO_NAMESPACES
}
#endif // NO_NAMESPACES

#ifdef TEST_PATHHASH
#include <stdio.h>
int main(int argc, char **argv)
{
    string path="/usr/lib/toto.cpp";
    string hash;
    pathHash(path, hash);
    printf("hash [%s]\n", hash.c_str());
	
}
#endif // TEST_PATHHASH
