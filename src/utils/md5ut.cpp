/* Copyright (C) 2015 J.F.Dockes
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

#ifndef TEST_MD5
#include <stdio.h>
#include <string.h>

#include "md5ut.h"

/* Convenience  utilities */

void MD5Final(string &digest, MD5_CTX *context)
{
    unsigned char d[16];
    MD5Final (d, context);
    digest.assign((const char *)d, 16);
}

string& MD5String(const string& data, string& digest)
{
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (const unsigned char*)data.c_str(), data.length());
    MD5Final(digest, &ctx);
    return digest;
}

string& MD5HexPrint(const string& digest, string &out)
{
    out.erase();
    out.reserve(33);
    static const char hex[]="0123456789abcdef";
    const unsigned char *hash = (const unsigned char *)digest.c_str();
    for (int i = 0; i < 16; i++) {
	out.append(1, hex[hash[i] >> 4]);
	out.append(1, hex[hash[i] & 0x0f]);
    }
    return out;
}
string& MD5HexScan(const string& xdigest, string& digest)
{
    digest.erase();
    if (xdigest.length() != 32) {
	return digest;
    }
    for (unsigned int i = 0; i < 16; i++) {
	unsigned int val;
	if (sscanf(xdigest.c_str() + 2*i, "%2x", &val) != 1) {
	    digest.erase();
	    return digest;
	}
	digest.append(1, (unsigned char)val);
    }
    return digest;
}

#include "readfile.h"
class FileScanMd5 : public FileScanDo {
public:
    FileScanMd5(string& d) : digest(d) {}
    virtual bool init(size_t size, string *)
    {
	MD5Init(&ctx);
	return true;
    }
    virtual bool data(const char *buf, int cnt, string*)
    {
	MD5Update(&ctx, (const unsigned char*)buf, cnt);
	return true;
    }
    string &digest;
    MD5_CTX ctx;
};
bool MD5File(const string& filename, string &digest, string *reason)
{
    FileScanMd5 md5er(digest);
    if (!file_scan(filename, &md5er, reason))
	return false;
    // We happen to know that digest and md5er.digest are the same object
    MD5Final(md5er.digest, &md5er.ctx);
    return true;
}

#else // TEST_MD5

// Test driver
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include "md5ut.h"

using namespace std;

static const char *thisprog;
static char usage [] =
"trmd5 filename\n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

int main(int argc, const char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

  if (argc != 1)
    Usage();
  string filename =  *argv++;argc--;

  string reason, digest;
  if (!MD5File(filename, digest, &reason)) {
      cerr << reason << endl;
      exit(1);
  } else {
      string hex;
      cout <<  "MD5 (" << filename << ") = " << MD5HexPrint(digest, hex) << endl;

      string digest1;
      MD5HexScan(hex, digest1);
      if (digest1.compare(digest)) {
	  cout << "MD5HexScan Failure" << endl;
	  cout <<  MD5HexPrint(digest, hex) << " " << digest.length() << " -> " 
	       << MD5HexPrint(digest1, hex) << " " << digest1.length() << endl;
	  exit(1);
      }

  }
  exit(0);
}

#endif
