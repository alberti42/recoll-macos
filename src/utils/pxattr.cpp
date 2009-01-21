/* @(#$Id: pxattr.cpp,v 1.9 2009-01-20 13:48:34 dockes Exp $  (C) 2009 J.F.Dockes
Copyright (c) 2009 Jean-Francois Dockes

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

// We want this to compile even to empty on non-supported systems. makes
// things easier for autoconf
#if defined(__FreeBSD__) || defined(__gnu_linux__) || defined(__APPLE__)

#ifndef TEST_PXATTR

#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(__FreeBSD__)
#include <sys/extattr.h>
#include <sys/uio.h>
#elif defined(__gnu_linux__)
#include <attr/xattr.h>
#elif defined(__APPLE__)
#include <sys/xattr.h>
#else
#error "Unknown system can't compile"
#endif

#include "pxattr.h"

namespace pxattr {

class AutoBuf {
public:
    char *buf;
    AutoBuf() : buf(0) {}
    ~AutoBuf() {if (buf) free(buf); buf = 0;}
    bool alloc(int n) 
    {
	if (buf) {
	    free(buf);
	    buf = 0;
	}
	buf = (char *)malloc(n); 
	return buf != 0;
    }
};

static bool 
get(int fd, const string& path, const string& _name, string *value,
    flags flags, nspace dom)
{
    string name;
    if (!sysname(dom, _name, &name)) 
	return false;

    ssize_t ret = -1;
    AutoBuf buf;

#if defined(__FreeBSD__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = extattr_get_link(path.c_str(), EXTATTR_NAMESPACE_USER, 
				   name.c_str(), 0, 0);
	} else {
	    ret = extattr_get_file(path.c_str(), EXTATTR_NAMESPACE_USER, 
				   name.c_str(), 0, 0);
	}
    } else {
	ret = extattr_get_fd(fd, EXTATTR_NAMESPACE_USER, name.c_str(), 0, 0);
    }
    if (ret < 0)
	return false;
    if (!buf.alloc(ret+1)) // Don't want to deal with possible ret=0
	return false;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = extattr_get_link(path.c_str(), EXTATTR_NAMESPACE_USER, 
				   name.c_str(), buf.buf, ret);
	} else {
	    ret = extattr_get_file(path.c_str(), EXTATTR_NAMESPACE_USER, 
				   name.c_str(), buf.buf, ret);
	}
    } else {
	ret = extattr_get_fd(fd, EXTATTR_NAMESPACE_USER, 
			     name.c_str(), buf.buf, ret);
    }
#elif defined(__gnu_linux__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = lgetxattr(path.c_str(), name.c_str(), 0, 0);
	} else {
	    ret = getxattr(path.c_str(), name.c_str(), 0, 0);
	}
    } else {
	ret = fgetxattr(fd, name.c_str(), 0, 0);
    }
    if (ret < 0)
	return false;
    if (!buf.alloc(ret+1)) // Don't want to deal with possible ret=0
	return false;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = lgetxattr(path.c_str(), name.c_str(), buf.buf, ret);
	} else {
	    ret = getxattr(path.c_str(), name.c_str(), buf.buf, ret);
	}
    } else {
	ret = fgetxattr(fd, name.c_str(), buf.buf, ret);
    }
#elif defined(__APPLE__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = getxattr(path.c_str(), name.c_str(), 0, 0, 0, XATTR_NOFOLLOW);
	} else {
	    ret = getxattr(path.c_str(), name.c_str(), 0, 0, 0, 0);
	}
    } else {
	ret = fgetxattr(fd, name.c_str(), 0, 0, 0, 0);
    }
    if (ret < 0)
	return false;
    if (!buf.alloc(ret+1)) // Don't want to deal with possible ret=0
	return false;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = getxattr(path.c_str(), name.c_str(), buf.buf, ret, 0, 
			   XATTR_NOFOLLOW);
	} else {
	    ret = getxattr(path.c_str(), name.c_str(), buf.buf, ret, 0, 0);
	}
    } else {
	ret = fgetxattr(fd, name.c_str(), buf.buf, ret, 0, 0);
    }
#endif

    if (ret >= 0)
	value->assign(buf.buf, ret);
    return ret >= 0;
}

static bool 
set(int fd, const string& path, const string& _name, 
    const string& value, flags flags, nspace dom)
{
    string name;
    if (!sysname(dom, _name, &name)) 
	return false;

    ssize_t ret = -1;

#if defined(__FreeBSD__)
    
    if (flags & (PXATTR_CREATE|PXATTR_REPLACE)) {
	// Need to test existence
	bool exists = false;
	ssize_t eret;
	if (fd < 0) {
	    if (flags & PXATTR_NOFOLLOW) {
		eret = extattr_get_link(path.c_str(), EXTATTR_NAMESPACE_USER, 
				       name.c_str(), 0, 0);
	    } else {
		eret = extattr_get_file(path.c_str(), EXTATTR_NAMESPACE_USER, 
				       name.c_str(), 0, 0);
	    }
	} else {
	    eret = extattr_get_fd(fd, EXTATTR_NAMESPACE_USER, 
				  name.c_str(), 0, 0);
	}
	if (eret >= 0)
	    exists = true;
	if (eret < 0 && errno != ENOATTR)
	    return false;
	if ((flags & PXATTR_CREATE) && exists) {
	    errno = EEXIST;
	    return false;
	}
	if ((flags & PXATTR_REPLACE) && !exists) {
	    errno = ENOATTR;
	    return false;
	}
    }
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = extattr_set_link(path.c_str(), EXTATTR_NAMESPACE_USER, 
				   name.c_str(), value.c_str(), value.length());
	} else {
	    ret = extattr_set_file(path.c_str(), EXTATTR_NAMESPACE_USER, 
				   name.c_str(), value.c_str(), value.length());
	}
    } else {
	ret = extattr_set_fd(fd, EXTATTR_NAMESPACE_USER, 
			     name.c_str(), value.c_str(), value.length());
    }
#elif defined(__gnu_linux__)
    int opts = 0;
    if (flags & PXATTR_CREATE)
	opts = XATTR_CREATE;
    else if (flags & PXATTR_REPLACE)
	opts = XATTR_REPLACE;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = lsetxattr(path.c_str(), name.c_str(), value.c_str(), 
			    value.length(), opts);
	} else {
	    ret = setxattr(path.c_str(), name.c_str(), value.c_str(), 
			   value.length(), opts);
	}
    } else {
	ret = fsetxattr(fd, name.c_str(), value.c_str(), value.length(), opts);
    }
#elif defined(__APPLE__)
    int opts = 0;
    if (flags & PXATTR_CREATE)
	opts = XATTR_CREATE;
    else if (flags & PXATTR_REPLACE)
	opts = XATTR_REPLACE;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = setxattr(path.c_str(), name.c_str(), value.c_str(), 
			   value.length(),  0, XATTR_NOFOLLOW|opts);
	} else {
	    ret = setxattr(path.c_str(), name.c_str(), value.c_str(), 
			   value.length(),  0, opts);
	}
    } else {
	ret = fsetxattr(fd, name.c_str(), value.c_str(), 
			value.length(), 0, opts);
    }
#endif
    return ret >= 0;
}

static bool 
del(int fd, const string& path, const string& _name, flags flags, nspace dom) 
{
    string name;
    if (!sysname(dom, _name, &name)) 
	return false;

    int ret = -1;

#if defined(__FreeBSD__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = extattr_delete_link(path.c_str(), EXTATTR_NAMESPACE_USER,
				      name.c_str());
	} else {
	    ret = extattr_delete_file(path.c_str(), EXTATTR_NAMESPACE_USER,
				      name.c_str());
	}
    } else {
	ret = extattr_delete_fd(fd, EXTATTR_NAMESPACE_USER, name.c_str());
    }
#elif defined(__gnu_linux__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = lremovexattr(path.c_str(), name.c_str());
	} else {
	    ret = removexattr(path.c_str(), name.c_str());
	}
    } else {
	ret = fremovexattr(fd, name.c_str());
    }
#elif defined(__APPLE__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = removexattr(path.c_str(), name.c_str(), XATTR_NOFOLLOW);
	} else {
	    ret = removexattr(path.c_str(), name.c_str(), 0);
	}
    } else {
	ret = fremovexattr(fd, name.c_str(), 0);
    }
#endif
    return ret >= 0;
}

static bool 
list(int fd, const string& path, vector<string>* names, flags flags, nspace dom)
{
    ssize_t ret = -1;
    AutoBuf buf;

#if defined(__FreeBSD__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = extattr_list_link(path.c_str(), EXTATTR_NAMESPACE_USER, 0, 0);
	} else {
	    ret = extattr_list_file(path.c_str(), EXTATTR_NAMESPACE_USER, 0, 0);
	}
    } else {
	ret = extattr_list_fd(fd, EXTATTR_NAMESPACE_USER, 0, 0);
    }
    if (ret < 0) 
	return false;
    if (!buf.alloc(ret+1)) // NEEDED on FreeBSD (no ending null)
	return false;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = extattr_list_link(path.c_str(), EXTATTR_NAMESPACE_USER, 
				    buf.buf, ret);
	} else {
	    ret = extattr_list_file(path.c_str(), EXTATTR_NAMESPACE_USER, 
				    buf.buf, ret);
	}
    } else {
	ret = extattr_list_fd(fd, EXTATTR_NAMESPACE_USER, buf.buf, ret);
    }
#elif defined(__gnu_linux__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = llistxattr(path.c_str(), 0, 0);
	} else {
	    ret = listxattr(path.c_str(), 0, 0);
	}
    } else {
	ret = flistxattr(fd, 0, 0);
    }
    if (ret < 0) 
	return false;
    if (!buf.alloc(ret+1)) // Don't want to deal with possible ret=0
	return false;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = llistxattr(path.c_str(), buf.buf, ret);
	} else {
	    ret = listxattr(path.c_str(), buf.buf, ret);
	}
    } else {
	ret = flistxattr(fd, buf.buf, ret);
    }
#elif defined(__APPLE__)
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = listxattr(path.c_str(), 0, 0, XATTR_NOFOLLOW);
	} else {
	    ret = listxattr(path.c_str(), 0, 0, 0);
	}
    } else {
	ret = flistxattr(fd, 0, 0, 0);
    }
    if (ret < 0) 
	return false;
    if (!buf.alloc(ret+1)) // Don't want to deal with possible ret=0
	return false;
    if (fd < 0) {
	if (flags & PXATTR_NOFOLLOW) {
	    ret = listxattr(path.c_str(), buf.buf, ret, XATTR_NOFOLLOW);
	} else {
	    ret = listxattr(path.c_str(), buf.buf, ret, 0);
	}
    } else {
	ret = flistxattr(fd, buf.buf, ret, 0);
    }
#endif

    char *bufstart = buf.buf;

    // All systems return a 0-separated string list except FreeBSD
    // which has length, value pairs, length is a byte. 
#if defined(__FreeBSD__)
    char *cp = buf.buf;
    unsigned int len;
    while (cp < buf.buf + ret + 1) {
	len = *cp;
	*cp = 0;
	cp += len + 1;
    }
    bufstart = buf.buf + 1;
    *cp = 0; // don't forget, we allocated one more
#endif


    if (ret > 0) {
	int pos = 0;
	while (pos < ret) {
	    string n = string(bufstart + pos);
	    string n1;
	    if (pxname(PXATTR_USER, n, &n1)) {
		names->push_back(n1);
	    }
	    pos += n.length() + 1;
	}
    }
    return true;
}

static const string nullstring("");

bool get(const string& path, const string& _name, string *value,
	 flags flags, nspace dom)
{
    return get(-1, path, _name, value, flags, dom);
}
bool get(int fd, const string& _name, string *value, flags flags, nspace dom)
{
    return get(fd, nullstring, _name, value, flags, dom);
}
bool set(const string& path, const string& _name, const string& value,
	 flags flags, nspace dom)
{
    return set(-1, path, _name, value, flags, dom);
}
bool set(int fd, const string& _name, const string& value, 
	 flags flags, nspace dom)
{
    return set(fd, nullstring, _name, value, flags, dom);
}
bool del(const string& path, const string& _name, flags flags, nspace dom) 
{
    return del(-1, path, _name, flags, dom);
}
bool del(int fd, const string& _name, flags flags, nspace dom) 
{
    return del(fd, nullstring, _name, flags, dom);
}
bool list(const string& path, vector<string>* names, flags flags, nspace dom)
{
    return list(-1, path, names, flags, dom);
}
bool list(int fd, vector<string>* names, flags flags, nspace dom)
{
    return list(fd, nullstring, names, flags, dom);
}

static const string userstring("user.");
bool sysname(nspace dom, const string& pname, string* sname)
{
    if (dom != PXATTR_USER) {
	errno = EINVAL;
	return false;
     }
    *sname = userstring + pname;
    return true;
}

bool pxname(nspace dom, const string& sname, string* pname) 
{
    if (sname.find("user.") != 0) {
	errno = EINVAL;
	return false;
    }
    *pname = sname.substr(userstring.length());
    return true;
}

} // namespace pxattr

#else // Testing / driver ->

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <iostream>

#include "pxattr.h"

static char *thisprog;
static char usage [] =
"pxattr [-h] -n name [-v value] pathname...\n"
"pxattr [-h] -x name pathname...\n"
"pxattr [-h] -l pathname...\n"
" [-h] : don't follow symbolic links (act on link itself)\n"
"pxattr -T: run tests on temp file in current directory" 
"\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_n	  0x2 
#define OPT_v	  0x4 
#define OPT_h     0x8
#define OPT_x     0x10
#define OPT_l     0x20
#define OPT_T     0x40

static void dotests()
{
    static const char *tfn = "pxattr_testtmp.xyz";
    static const char *NAMES[] = {"ORG.PXATTR.NAME1", "ORG.PXATTR.N2", 
				  "ORG.PXATTR.LONGGGGGGGGisSSSHHHHHHHHHNAME3"};
    static const char *VALUES[] = {"VALUE1", "VALUE2", "VALUE3"};
    static bool verbose = true;

    /* Create test file if it doesn't exist, remove all attributes */
    int fd = open(tfn, O_RDWR|O_CREAT, 0755);
    if (fd < 0) {
	perror("open/create");
	exit(1);
    }


    if (verbose)
	fprintf(stdout, "Cleanup old attrs\n");
    vector<string> names;
    if (!pxattr::list(tfn, &names)) {
	perror("pxattr::list");
	exit(1);
    }
    for (vector<string>::const_iterator it = names.begin(); 
	 it != names.end(); it++) {
	string value;
	if (!pxattr::del(fd, *it)) {
	    perror("pxattr::del");
	    exit(1);
	}
    }
    /* Check that there are no attributes left */
    names.clear();
    if (!pxattr::list(tfn, &names)) {
	perror("pxattr::list");
	exit(1);
    }
    if (names.size() != 0) {
	fprintf(stderr, "Attributes remain after initial cleanup !\n");
	for (vector<string>::const_iterator it = names.begin();
	     it != names.end(); it++) {
	    fprintf(stderr, "%s\n", (*it).c_str());
	}
	exit(1);
    }

    /* Create attributes, check existence and value */
    if (verbose)
	fprintf(stdout, "Creating extended attributes\n");
    for (int i = 0; i < 3; i++) {
	if (!pxattr::set(fd, NAMES[i], VALUES[i])) {
	    perror("pxattr::set");
	    exit(1);
	}
    }
    if (verbose)
	fprintf(stdout, "Checking creation\n");
    for (int i = 0; i < 3; i++) {
	string value;
	if (!pxattr::get(tfn, NAMES[i], &value)) {
	    perror("pxattr::get");
	    exit(1);
	}
	if (value.compare(VALUES[i])) {
	    fprintf(stderr, "Wrong value after create !\n");
	    exit(1);
	}
    }

    /* Delete one, check list */
    if (verbose)
	fprintf(stdout, "Delete one\n");
    if (!pxattr::del(tfn, NAMES[1])) {
	perror("pxattr::del one name");
	exit(1);
    }
    if (verbose)
	fprintf(stdout, "Check list\n");
    for (int i = 0; i < 3; i++) {
	string value;
	if (!pxattr::get(fd, NAMES[i], &value)) {
	    if (i == 1)
		continue;
	    perror("pxattr::get");
	    exit(1);
	} else if (i == 1) {
	    fprintf(stderr, "Name at index 1 still exists after deletion\n");
	    exit(1);
	}
	if (value.compare(VALUES[i])) {
	    fprintf(stderr, "Wrong value after delete 1 !\n");
	    exit(1);
	}
    }

    /* Test the CREATE/REPLACE flags */
    // Set existing with flag CREATE should fail
    if (verbose)
	fprintf(stdout, "Testing CREATE/REPLACE flags use\n");
    if (pxattr::set(tfn, NAMES[0], VALUES[0], pxattr::PXATTR_CREATE)) {
	fprintf(stderr, "Create existing with flag CREATE succeeded !\n");
	exit(1);
    }
    // Set new with flag REPLACE should fail
    if (pxattr::set(tfn, NAMES[1], VALUES[1], pxattr::PXATTR_REPLACE)) {
	fprintf(stderr, "Create new with flag REPLACE succeeded !\n");
	exit(1);
    }
    // Set new with flag CREATE should succeed
    if (!pxattr::set(fd, NAMES[1], VALUES[1], pxattr::PXATTR_CREATE)) {
	fprintf(stderr, "Create new with flag CREATE failed !\n");
	exit(1);
    }
    // Set existing with flag REPLACE should succeed
    if (!pxattr::set(fd, NAMES[0], VALUES[0], pxattr::PXATTR_REPLACE)) {
	fprintf(stderr, "Create existing with flag REPLACE failed !\n");
	exit(1);
    }
    close(fd);
    unlink(tfn);
    exit(0);
}

static void listattrs(const string& path)
{
    std::cout << "Path: " << path << std::endl;
    vector<string> names;
    if (!pxattr::list(path, &names)) {
	perror("pxattr::list");
	exit(1);
    }
    for (vector<string>::const_iterator it = names.begin(); 
	 it != names.end(); it++) {
	string value;
	if (!pxattr::get(path, *it, &value)) {
	    perror("pxattr::get");
	    exit(1);
	}
	std::cout << " " << *it << " => " << value << std::endl;
    }
}

void setxattr(const string& path, const string& name, const string& value)
{
    if (!pxattr::set(path, name, value)) {
	perror("pxattr::set");
	exit(1);
    }
}

void  printxattr(const string &path, const string& name)
{
    std::cout << "Path: " << path << std::endl;
    string value;
    if (!pxattr::get(path, name, &value)) {
	perror("pxattr::get");
	exit(1);
    }
    std::cout << " " << name << " => " << value << std::endl;
}

void delxattr(const string &path, const string& name) 
{
    if (pxattr::del(path, name) < 0) {
	perror("pxattr::del");
	exit(1);
    }
}


int main(int argc, char **argv)
{
  thisprog = argv[0];
  argc--; argv++;

  string name, value;

  while (argc > 0 && **argv == '-') {
    (*argv)++;
    if (!(**argv))
      /* Cas du "adb - core" */
      Usage();
    while (**argv)
      switch (*(*argv)++) {
      case 'T':	op_flags |= OPT_T; break;
      case 'l':	op_flags |= OPT_l; break;
      case 'x':	op_flags |= OPT_x; if (argc < 2)  Usage();
	  name = *(++argv); argc--; 
	goto b1;
      case 'n':	op_flags |= OPT_n; if (argc < 2)  Usage();
	  name = *(++argv); argc--; 
	goto b1;
      case 'v':	op_flags |= OPT_v; if (argc < 2)  Usage();
	  value = *(++argv); argc--; 
	goto b1;
      default: Usage();	break;
      }
  b1: argc--; argv++;
  }

  if (argc < 1 && !(op_flags & OPT_T))
    Usage();
  if (op_flags & OPT_l) {
      while (argc > 0) {
	  listattrs(*argv++);argc--;
      } 
  } else if (op_flags & OPT_n) {
      if (op_flags & OPT_v) {
	  while (argc > 0) {
	      setxattr(*argv++, name, value);argc--;
	  } 
      } else {
	  while (argc > 0) {
	      printxattr(*argv++, name);argc--;
	  } 
      }
  } else if (op_flags & OPT_x) {
      while (argc > 0) {
	  delxattr(*argv++, name);argc--;
      } 
  } else if (op_flags & OPT_T)  {
      dotests();
  }
  exit(0);
}


#endif // Testing pxattr

#endif // Supported systems.
