#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.1 2004-12-14 17:50:28 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "rcldb.h"

#include "xapian.h"

// Data for a xapian database
class Native {
 public:
    bool isopen;
    bool iswritable;
    class Xapian::Database db;
    class Xapian::WritableDatabase wdb;
    vector<bool> updated;

    Native() : isopen(false), iswritable(false) {}

};

Rcl::Db::Db() 
{
    pdata = new Native;
}

Rcl::Db::~Db()
{
    if (pdata == 0)
	return;
    Native *ndb = (Native *)pdata;
    try {
	// There is nothing to do for an ro db.
	if (ndb->isopen == false || ndb->iswritable == false) {
	    delete ndb;
	    return;
	}
	ndb->wdb.flush();
	delete ndb;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
    }
}

bool Rcl::Db::open(const string& dir, OpenMode mode)
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    try {
	switch (mode) {
	case DbUpd:
	    ndb->wdb = Xapian::Auto::open(dir, Xapian::DB_CREATE_OR_OPEN);
	    ndb->updated.resize(ndb->wdb.get_lastdocid() + 1);
	    ndb->iswritable = true;
	    break;
	case DbTrunc:
	    ndb->wdb = Xapian::Auto::open(dir, Xapian::DB_CREATE_OR_OVERWRITE);
	    ndb->iswritable = true;
	    break;
	case DbRO:
	default:
	    ndb->iswritable = false;
	    cerr << "Not ready to open RO yet" << endl;
	    exit(1);
	}
	ndb->isopen = true;
	return true;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
    }
    return false;
}
bool Rcl::Db::close()
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    if (ndb->isopen == false)
	return true;
    try {
	if (ndb->isopen == true && ndb->iswritable == true) {
	    ndb->wdb.flush();
	}
	delete ndb;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
	return false;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
	return false;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
	return false;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
	return false;
    }
    pdata = new Native;
    if (pdata)
	return true;
    return false;
}
