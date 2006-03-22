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
#ifndef _CONFTREE_H_
#define  _CONFTREE_H_

/**
 * A simple configuration file implementation.
 *
 * Configuration files have lines like 'name = value', and/or like '[subkey]'
 *
 * Lines like '[subkeyname]' in the file define subsections, with independant
 * configuration namespaces.
 *
 * Whitespace around name and value is insignificant.
 *
 * Values can be queried for, or set. (the file is then rewritten).
 * The names are case-sensitive but don't count on it either.
 * Any line without a '=' is discarded when rewriting the file.
 * All 'set' calls currently cause an immediate file rewrite.
 */

#include <string>
#include <map>
#include <list>
// rh7.3 likes iostream better...
#if defined(__GNUC__) && __GNUC__ < 3
#include <iostream>
#else
#include <istream>
#endif

#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::map;
#endif // NO_NAMESPACES

/** 
 * Manages a simple configuration file with subsections.
 */
class ConfSimple {
 public:
    enum StatusCode {STATUS_ERROR=0, STATUS_RO=1, STATUS_RW=2};

    /**
     * Build the object by reading content from file.
     * @param filename file to open
     * @param readonly if true open readonly, else rw
     * @param tildexp  try tilde (home dir) expansion for subkey values
     */
    ConfSimple(const char *fname, int readonly = 0, bool tildexp = false);

    /**
     * Build the object by reading content from a string
     * @param data points to the data to parse
     * @param readonly if true open readonly, else rw
     * @param tildexp  try tilde (home dir) expansion for subsection names
     */
    ConfSimple(string *data, int readonly = 0, bool tildexp = false);

    virtual ~ConfSimple() {};

    /** 
     * Get value for named parameter, from specified subsection (looks in 
     * global space if sk is empty).
     * @return 0 if name not found, 1 else
     */
    virtual int get(const std::string &name, string &value, 
		    const string &sk = string(""));
    /* Note: the version returning char* was buggy and has been removed */

    /** 
     * Set value for named parameter in specified subsection (or global)
     * @return 0 for error, 1 else
     */
    virtual int set(const std::string &nm, const std::string &val, 
	    const std::string &sk);
    virtual int set(const char *name, const char *value, const char *sk = 0);

    /**
     * Remove name and value from config
     */
    virtual int erase(const std::string &name, const std::string &sk);

    virtual StatusCode getStatus(); 
    virtual bool ok() {return getStatus() != STATUS_ERROR;}

    /** 
     * Walk the configuration values, calling function for each.
     * The function is called with a null nm when changing subsections (the 
     * value is then the new subsection name)
     * @return WALK_STOP when/if the callback returns WALK_STOP, 
     *         WALK_CONTINUE else (got to end of config)
     */
    enum WalkerCode {WALK_STOP, WALK_CONTINUE};
    virtual WalkerCode sortwalk(WalkerCode 
				(*wlkr)(void *cldata, const std::string &nm, 
					const std::string &val),
				void *clidata);
    virtual void list();

    /**
     * Return all names in given submap
     */
    virtual std::list<string> getNames(const string &sk);

    virtual std::string getFilename() {return filename;}

    /**
     * Copy constructor. Expensive but less so than a full rebuild
     */
    ConfSimple(const ConfSimple &rhs) : data(0) {
	if ((status = rhs.status) == STATUS_ERROR)
	    return;
	filename = rhs.filename;
	if (rhs.data) {
	    data = new string(*(rhs.data));
	}
	submaps = rhs.submaps;
    }

    /**
     * Assignement. This is expensive
     */
    ConfSimple& operator=(const ConfSimple &rhs) {
	if (this != &rhs && (status = rhs.status) != STATUS_ERROR) {
	    delete data;
	    data = 0;
	    filename = rhs.filename;
	    if (rhs.data) {
		data = new string(*(rhs.data));
	    }
	    submaps = rhs.submaps;
	}
	return *this;
    }

 protected:
    bool dotildexpand;
    StatusCode status;
 private:
    string filename; // set if we're working with a file
    string *data;    // set if we're working with an in-memory string
    map<string, map<string, string> > submaps;

    void parseinput(std::istream &input);
};

/**
 * This is a configuration class which attaches tree-like signification to the
 * submap names.
 *
 * If a given variable is not found in the specified section, it will be 
 * looked up the tree of section names, and in the global space.
 *
 * submap names should be '/' separated paths (ie: /sub1/sub2). No checking
 * is done, but else the class adds no functionality to ConfSimple.
 *
 * NOTE: contrary to common behaviour, the global or root space is NOT
 * designated by '/' but by '' (empty subkey). A '/' subkey will not
 * be searched at all.
 */
class ConfTree : public ConfSimple {

 public:
    /**
     * Build the object by reading content from file.
     */
    ConfTree(const char *fname, int readonly = 0) 
	: ConfSimple(fname, readonly, true) {}
    virtual ~ConfTree() {};
    ConfTree(const ConfTree& r)	: ConfSimple(r) {};
    ConfTree& operator=(const ConfTree& r) {
	ConfSimple::operator=(r);
	return *this;
    }

    /** 
     * Get value for named parameter, from specified subsection, or its 
     * parents.
     * @return 0 if name not found, 1 else
     */
    virtual int get(const std::string &name, string &value, const string &sk);

    virtual int get(const char *name, string &value, const char *sk) {
	return get(string(name), value, sk ? string(sk) : string(""));
    }
};

/** 
 * Use several config files, trying to get values from each in order. Used to
 * have a central config, with possible overrides from more specific
 * (ie personal) ones.
 *
 * Notes: it's ok for some of the files in the list to not exist, but the last
 * one must or we generate an error. We open all trees readonly.
 */
template <class T> class ConfStack {
 public:
    ConfStack(const std::list<string> &fns, bool ro = true) {
	construct(fns, ro);
    }

    ConfStack(const char *nm, bool ro = true) {
	std::list<string> fns;
	fns.push_back(string(nm));
	construct(fns, ro);
    }

    ~ConfStack() {
	erase();
	m_ok = false;
    }

    ConfStack(const ConfStack &rhs) {
	init_from(rhs);
    }

    ConfStack& operator=(const ConfStack &rhs) {
	if (this != &rhs){
	    erase();
	    m_ok = rhs.m_ok;
	    if (m_ok)
		init_from(rhs);
	}
	return *this;
    }

    int get(const std::string &name, string &value, const string &sk) {
	typename std::list<T*>::iterator it;
	for (it = m_confs.begin();it != m_confs.end();it++) {
	    if ((*it)->get(name, value, sk))
		return true;
	}
	return false;
    }

    int get(const char *name, string &value, const char *sk) {
	return get(string(name), value, sk ? string(sk) : string(""));
    }

    std::list<string> getNames(const string &sk) {
	std::list<string> nms;
	typename std::list<T*>::iterator it;
	for (it = m_confs.begin();it != m_confs.end();it++) {
	    std::list<string> lst;
	    lst = (*it)->getNames(sk);
	    nms.splice(nms.end(), lst);
	}
	return nms;
    }

    bool ok() {return m_ok;}

 private:
    bool m_ok;
    std::list<T*> m_confs;
    
    void erase() {
	typename std::list<T*>::iterator it;
	for (it = m_confs.begin();it != m_confs.end();it++) {
	    delete (*it);
	}
	m_confs.clear();
    }

    /// Common code to initialize from existing object
    void init_from(const ConfStack &rhs) {
	if ((m_ok = rhs.m_ok)) {
	    typename std::list<T*>::const_iterator it;
	    for (it = rhs.m_confs.begin();it != rhs.m_confs.end();it++) {
		m_confs.push_back(new T(**it));
	    }
	}
    }

    /// Common constructor code
    void construct(const std::list<string> &fns, bool ro) {
	if (!ro) {
	    m_ok = false;
	    return;
	}
	std::list<std::string>::const_iterator it;
	bool lastok = false;
	for (it = fns.begin();it != fns.end();it++) {
	    T* p = new T(it->c_str(), true);
	    if (p && p->ok()) {
		m_confs.push_back(p);
		lastok = true;
	    } else
		lastok = false;
	}
	m_ok = lastok;
    }
};

#endif /*_CONFTREE_H_ */
