#ifndef lint
static char rcsid[] = "@(#$Id: pathut.cpp,v 1.4 2005-02-04 14:21:18 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_PATHUT
#include <unistd.h>
#include <pwd.h>
#include <iostream>

#include "pathut.h"
using std::string;

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

#else // TEST_PATHUT

#include <iostream>
using namespace std;

#include "pathut.h"

const char *tstvec[] = {"", "/", "/dir", "/dir/", "/dir1/dir2",
			 "/dir1/dir2",
			"./dir", "./dir1/", "dir", "../dir", "/dir/toto.c",
			"/dir/.c",
};

const string ttvec[] = {"/dir", "", "~", "~/sub", "~root", "~root/sub",
		 "~nosuch", "~nosuch/sub"};
int nttvec = sizeof(ttvec) / sizeof(string);

int main(int argc, const char **argv)
{
#if 0
    for (int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " FATHER " << path_getfather(tstvec[i]) << endl;
    }
    for (int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " SIMPLE " << path_getsimple(tstvec[i]) << endl;
    }
#endif
    string s;

    for (int i = 0; i < nttvec; i++) {
	cout << "tildexp: '" << ttvec[i] << "' -> '" << 
	    path_tildexpand(ttvec[i]) << "'" << endl;
    }
    


    return 0;
}

#endif // TEST_PATHUT

