#ifndef lint
static char rcsid[] = "@(#$Id: pyrecoll.cpp,v 1.3 2008-06-13 18:22:46 dockes Exp $ (C) 2007 J.F.Dockes";
#endif

#include <Python.h>

#include <string>
#include <iostream>
using namespace std;

#include "rclinit.h"
#include "rclconfig.h"
#include "rcldb.h"
#include "rclquery.h"
#include "pathut.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"

static RclConfig *config;

static PyObject *
recollq_question(PyObject *self, PyObject *args)
{
    int limit = 100;
    const char *qs;
    if (!PyArg_ParseTuple(args, "s", &qs)) {
	PyErr_SetString(PyExc_EnvironmentError, "Bad query syntax");
        return NULL;
    }

    Rcl::Db rcldb;
    string reason;
    string dbdir = config->getDbDir();
    rcldb.open(dbdir, config->getStopfile(), 
	       Rcl::Db::DbRO);

    Rcl::SearchData *sd = wasaStringToRcl(qs, reason);
    if (!sd) {
	PyErr_SetString(PyExc_EnvironmentError, reason.c_str());
	return 0;
    }

    RefCntr<Rcl::SearchData> rq(sd);
    RefCntr<Rcl::Query> query(new Rcl::Query(&rcldb));
    query->setQuery(rq, Rcl::Query::QO_STEM);
    int cnt = query->getResCnt();
    cout << "Recoll query: " << rq->getDescription() << endl;
    if (cnt <= limit)
	cout << cnt << " results" << endl;
    else
	cout << cnt << " results (printing  " << limit << " max):" << endl;

    for (int i = 0; i < limit; i++) {
	int pc;
	Rcl::Doc doc;
	if (!query->getDoc(i, doc, &pc))
	    break;
	char cpc[20];
	sprintf(cpc, "%d", pc);
	cout 
	    << doc.mimetype.c_str() << "\t"
	    << "[" << doc.url.c_str() << "]" << "\t" 
	    << "[" << doc.meta["title"].c_str() << "]" << "\t"
	    << doc.fbytes.c_str()   << "\tbytes" << "\t"
	    <<  endl;
    }
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef recollqMethods[] = {
    {"question",  recollq_question, METH_VARARGS, "Query language query."},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};


RclConfig *RclConfig::getMainConfig() 
{
    return config;
}

PyMODINIT_FUNC
initrecollq(void)
{
    (void) Py_InitModule("recollq", recollqMethods);

    string reason;
    config = recollinit(0, 0, reason, 0);
    if (config == 0) {
	PyErr_SetString(PyExc_EnvironmentError, reason.c_str());
	return;
    }
    if (!config->ok()) {
	PyErr_SetString(PyExc_EnvironmentError, 
			"Recoll init error: bad environment ?");
	return;
    }
    fprintf(stderr, "initrecollq ok\n");
}

