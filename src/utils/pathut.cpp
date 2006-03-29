#ifndef lint
static char rcsid[] = "@(#$Id: pathut.cpp,v 1.10 2006-03-29 11:18:15 dockes Exp $ (C) 2004 J.F.Dockes";
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

#ifndef TEST_PATHUT
#include <unistd.h>
#include <sys/param.h>
#include <pwd.h>

#include <iostream>
#include <list>
#include <stack>

#include "pathut.h"
#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::stack;
#endif /* NO_NAMESPACES */

void path_catslash(std::string &s) {
    if (s.empty() || s[s.length() - 1] != '/')
	s += '/';
}

std::string path_cat(const std::string &s1, const std::string &s2) {
    std::string res = s1;
    path_catslash(res);
    res +=  s2;
    return res;
}

string path_getfather(const string &s) {
    string father = s;

    // ??
    if (father.empty())
	return "./";

    if (father[father.length() - 1] == '/') {
	// Input ends with /. Strip it, handle special case for root
	if (father.length() == 1)
	    return father;
	father.erase(father.length()-1);
    }

    string::size_type slp = father.rfind('/');
    if (slp == string::npos)
	return "./";

    father.erase(slp);
    path_catslash(father);
    return father;
}

string path_getsimple(const string &s) {
    string simple = s;

    if (simple.empty())
	return simple;

    string::size_type slp = simple.rfind('/');
    if (slp == string::npos)
	return simple;

    simple.erase(0, slp+1);
    return simple;
}

string path_basename(const string &s, const string &suff)
{
    string simple = path_getsimple(s);
    string::size_type pos = string::npos;
    if (suff.length() && simple.length() > suff.length()) {
	pos = simple.rfind(suff);
	if (pos != string::npos && pos + suff.length() == simple.length())
	    return simple.substr(0, pos);
    } 
    return simple;
}

string path_home()
{
    uid_t uid = getuid();

    struct passwd *entry = getpwuid(uid);
    if (entry == 0) {
	const char *cp = getenv("HOME");
	if (cp)
	    return cp;
	else 
	return "/";
    }

    string homedir = entry->pw_dir;
    path_catslash(homedir);
    return homedir;
}

extern string path_tildexpand(const string &s) 
{
    if (s.empty() || s[0] != '~')
	return s;
    string o = s;
    if (s.length() == 1) {
	o.replace(0, 1, path_home());
    } else if  (s[1] == '/') {
	o.replace(0, 2, path_home());
    } else {
	string::size_type pos = s.find('/');
	int l = (pos == string::npos) ? s.length() - 1 : pos - 1;
	struct passwd *entry = getpwnam(s.substr(1, l).c_str());
	if (entry)
	    o.replace(0, l+1, entry->pw_dir);
    }
    return o;
}

#include <smallut.h>
extern std::string path_canon(const std::string &is)
{
    if (is.length() == 0)
	return is;
    string s = is;
    if (s[0] != '/') {
	char buf[MAXPATHLEN];
	if (!getcwd(buf, MAXPATHLEN)) {
	    return "";
	}
	s = path_cat(string(buf), s); 
    }
    list<string>elems;
    stringToTokens(s, elems, "/");
    list<string> cleaned;
    for (list<string>::const_iterator it = elems.begin(); 
	 it != elems.end(); it++){
	if (*it == "..") {
	    if (!cleaned.empty())
		cleaned.pop_back();
	} else if (it->empty() || *it == ".") {
	} else {
	    cleaned.push_back(*it);
	}
    }
    string ret;
    if (!cleaned.empty()) {
	for (list<string>::const_iterator it = cleaned.begin(); 
	     it != cleaned.end(); it++) {
	    ret += "/";
	    ret += *it;
	}
    } else {
	ret = "/";
    }
    return ret;
}

#include <glob.h>
#include <sys/stat.h>
list<std::string> path_dirglob(const std::string &dir, 
				    const std::string pattern)
{
    list<string> res;
    glob_t mglob;
    string mypat=path_cat(dir, pattern);
    if (glob(mypat.c_str(), 0, 0, &mglob)) {
	return res;
    }
    for (int i = 0; i < int(mglob.gl_pathc); i++) {
	res.push_back(mglob.gl_pathv[i]);
    }
    globfree(&mglob);
    return res;
}

std::string url_encode(const std::string url, string::size_type offs)
{
    string out = url.substr(0, offs);
    const char *cp = url.c_str();
    for (string::size_type i = offs; i < url.size(); i++) {
	int c;
	char *h = "0123456789ABCDEF";
	c = cp[i];
	if(c <= 0x1f || 
	   c >= 0x7f || 
	   c == '<' ||
	   c == '>' ||
	   c == ' ' ||
	   c == '\t'||
	   c == '"' ||
	   c == '#' ||
	   c == '%' ||
	   c == '{' ||
	   c == '}' ||
	   c == '|' ||
	   c == '\\' ||
	   c == '^' ||
	   c == '~'||
	   c == '[' ||
	   c == ']' ||
	   c == '`') {
	    out += '%';
	    out += h[(c >> 4) & 0xf];
	    out += h[c & 0xf];
	} else {
	    out += char(c);
	}
    }
    return out;
}

#else // TEST_PATHUT

#include <iostream>
using namespace std;

#include "pathut.h"

const char *tstvec[] = {"", "/", "/dir", "/dir/", "/dir1/dir2",
			 "/dir1/dir2",
			"./dir", "./dir1/", "dir", "../dir", "/dir/toto.c",
			"/dir/.c", "/dir/toto.txt", "toto.txt1"
};

const string ttvec[] = {"/dir", "", "~", "~/sub", "~root", "~root/sub",
		 "~nosuch", "~nosuch/sub"};
int nttvec = sizeof(ttvec) / sizeof(string);

int main(int argc, const char **argv)
{
    string s;
    list<string>::const_iterator it;
#if 0
    for (unsigned int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " Father " << path_getfather(tstvec[i]) << endl;
    }
    for (unsigned int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " Simple " << path_getsimple(tstvec[i]) << endl;
    }
    for (unsigned int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " Basename " << 
	    path_basename(tstvec[i], ".txt") << endl;
    }
#endif

#if 0
    for (int i = 0; i < nttvec; i++) {
	cout << "tildexp: '" << ttvec[i] << "' -> '" << 
	    path_tildexpand(ttvec[i]) << "'" << endl;
    }
#endif

#if 0
    const string canontst[] = {"/dir1/../../..", "/////", "", 
			       "/dir1/../../.././/////dir2///////",
			       "../../", 
			       "../../../../../../../../../../"
    };
    unsigned int nttvec = sizeof(canontst) / sizeof(string);
    for (unsigned int i = 0; i < nttvec; i++) {
	cout << "canon: '" << canontst[i] << "' -> '" << 
	    path_canon(canontst[i]) << "'" << endl;
    }
#endif    
#if 1
    if (argc != 3) {
	fprintf(stderr, "Usage: trpathut <dir> <pattern>\n");
	exit(1);
    }
    string dir=argv[1], pattern=argv[2];
    list<string> matched = path_dirglob(dir, pattern);
    for (it = matched.begin(); it != matched.end();it++) {
	cout << *it << endl;
    }
#endif

    return 0;
}

#endif // TEST_PATHUT

