#ifndef lint
static char rcsid[] = "@(#$Id: pyrecoll.cpp,v 1.6 2008-08-26 07:36:41 dockes Exp $ (C) 2007 J.F.Dockes";
#endif

#include <Python.h>
#include <structmember.h>

#include <string>
#include <iostream>
#include <set>
using namespace std;

#include "rclinit.h"
#include "rclconfig.h"
#include "rcldb.h"
#include "rclquery.h"
#include "pathut.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"
#include "debuglog.h"
#include "pathut.h"

static set<Rcl::Db *> the_dbs;
static set<Rcl::Query *> the_queries;
static set<Rcl::Doc *> the_docs;

static RclConfig *rclconfig;

// This has to exist somewhere in the python api ??
PyObject *obj_Create(PyTypeObject *tp, PyObject *args, PyObject *kwargs)
{
    PyObject *result = tp->tp_new(tp, args, kwargs);
    if (result && tp->tp_init(result, args, kwargs) < 0)
	return 0;
    return result;
}

//////////////////////////////////////////////////////
////// Python object definitions for Db, Query, and Doc
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Rcl::Db *db;
} recollq_DbObject;
static PyTypeObject recollq_DbType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recollq.Db",             /*tp_name*/
    sizeof(recollq_DbObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Recollq Db objects",      /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};


typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Rcl::Query *query;
    int         next; // Index of result to be fetched next or -1 if uninit
} recollq_QueryObject;

static PyTypeObject recollq_QueryType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recollq.Query",             /*tp_name*/
    sizeof(recollq_QueryObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Recollq Query object",    /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Rcl::Doc *doc;
} recollq_DocObject;

static PyTypeObject recollq_DocType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recollq.Doc",             /*tp_name*/
    sizeof(recollq_DocObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Recollq Doc objects",     /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

///////////////////////////////////////////////
////// Db object code
static void 
Db_dealloc(recollq_DbObject *self)
{
    LOGDEB(("Db_dealloc\n"));
    if (self->db)
	the_dbs.erase(self->db);
    delete self->db;
    self->db = 0;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Db_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LOGDEB(("Db_new\n"));
    recollq_DbObject *self;

    self = (recollq_DbObject *)type->tp_alloc(type, 0);
    if (self == 0) 
	return 0;
    self->db = 0;
    return (PyObject *)self;
}

static int
Db_init(recollq_DbObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("Db_init\n"));
    static char *kwlist[] = {"confdir", "extra_dbs", "writable", NULL};
    PyObject *extradbs = 0;
    char *confdir = 0;
    int writable = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sOi", kwlist,
				     &confdir, &extradbs, &writable))
	return -1;

    // If the user creates several dbs, changing the confdir, we call 
    // recollinit repeatedly, which *should* be ok...
    string reason;
    delete rclconfig;
    if (confdir) {
	string cfd = confdir;
	rclconfig = recollinit(0, 0, reason, &cfd);
    } else {
	rclconfig = recollinit(0, 0, reason, 0);
    }
    if (rclconfig == 0) {
	PyErr_SetString(PyExc_EnvironmentError, reason.c_str());
	return -1;
    }
    if (!rclconfig->ok()) {
	PyErr_SetString(PyExc_EnvironmentError, "Bad config ?");
	return -1;
    }

    if (self->db)
	the_dbs.erase(self->db);
    delete self->db;
    self->db = new Rcl::Db;
    string dbdir = rclconfig->getDbDir();
    LOGDEB(("Db_init: getdbdir ok: [%s]\n", dbdir.c_str()));
    if (!self->db->open(dbdir, rclconfig->getStopfile(), writable ? 
			Rcl::Db::DbUpd : Rcl::Db::DbRO)) {
	LOGDEB(("Db_init: db open error\n"));
	PyErr_SetString(PyExc_EnvironmentError, "Can't open index");
        return -1;
    }

    if (extradbs) {
	if (!PySequence_Check(extradbs)) {
	    PyErr_SetString(PyExc_TypeError, "extra_dbs must be a sequence");
	    deleteZ(self->db);
	    return -1;
	}
	int dbcnt = PySequence_Size(extradbs);
	if (dbcnt == -1) {
	    PyErr_SetString(PyExc_TypeError, "extra_dbs could not be sized");
	    deleteZ(self->db);
	    return -1;
	}
	for (int i = 0; i < dbcnt; i++) {
	    PyObject *item = PySequence_GetItem(extradbs, i);
	    char *s = PyString_AsString(item);
	    Py_DECREF(item);
	    if (!s) {
		PyErr_SetString(PyExc_TypeError,
				"extra_dbs must contain strings");
		deleteZ(self->db);
		return -1;
	    }
	    if (!self->db->addQueryDb((const char *)s)) {
		PyErr_SetString(PyExc_EnvironmentError, 
				"extra db could not be opened");
		deleteZ(self->db);
		return -1;
	    }
	}
    }

    the_dbs.insert(self->db);
    LOGDEB(("Db_init: done: db %p\n", self->db));
    return 0;
}

static PyObject *
Db_query(recollq_DbObject* self)
{
    LOGDEB(("Db_query\n"));
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGDEB(("Db_query: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db");
        return 0;
    }
    recollq_QueryObject *result = 
	(recollq_QueryObject *)obj_Create(&recollq_QueryType, 0, 0);
    if (!result)
	return 0;
    result->query = new Rcl::Query(self->db);
    the_queries.insert(result->query);
    LOGDEB(("Db_query: done\n"));
    return (PyObject *)result;
}

static PyObject *
Db_setAbstractParams(recollq_DbObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("Db_setAbstractParams\n"));
    static char *kwlist[] = {"maxchars", "contextwords", NULL};
    int ctxwords = -1, maxchars = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ii", kwlist,
				     &maxchars, &ctxwords))
	return 0;
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGDEB(("Db_query: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db");
        return 0;
    }
    self->db->setAbstractParams(-1, maxchars, ctxwords);
    Py_RETURN_NONE;
}

static PyObject *
Db_makeDocAbstract(recollq_DbObject* self, PyObject *args, PyObject *)
{
    LOGDEB(("Db_makeDocAbstract\n"));
    recollq_DocObject *pydoc;
    recollq_QueryObject *pyquery;
    if (!PyArg_ParseTuple(args, "O!O!:Db_makeDocAbstract",
			  &recollq_DocType, &pydoc,
			  &recollq_QueryType, &pyquery)) {
	return 0;
    }
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGERR(("Db_makeDocAbstract: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db");
        return 0;
    }
    if (pydoc->doc == 0 || the_docs.find(pydoc->doc) == the_docs.end()) {
	LOGERR(("Db_makeDocAbstract: doc not found %p\n", pydoc->doc));
        PyErr_SetString(PyExc_AttributeError, "doc");
        return 0;
    }
    if (pyquery->query == 0 || 
	the_queries.find(pyquery->query) == the_queries.end()) {
	LOGERR(("Db_makeDocAbstract: query not found %p\n", pyquery->query));
        PyErr_SetString(PyExc_AttributeError, "query");
        return 0;
    }
    string abstract;
    if (!self->db->makeDocAbstract(*(pydoc->doc), pyquery->query, abstract)) {
	PyErr_SetString(PyExc_EnvironmentError, "rcl makeDocAbstract failed");
        return 0;
    }
    // Return a python unicode object
    return PyUnicode_Decode(abstract.c_str(), abstract.size(), 
				     "UTF-8", "replace");
}

static PyObject *
Db_needUpdate(recollq_DbObject* self, PyObject *args, PyObject *kwds)
{
    char *udi = 0;
    char *sig = 0;
    LOGDEB(("Db_needUpdate\n"));
    if (!PyArg_ParseTuple(args, "eses:Db_needUpdate", 
			  "utf-8", &udi, "utf-8", &sig)) {
	return 0;
    }
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGERR(("Db_makeDocAbstract: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db");
        return 0;
    }
    bool result = self->db->needUpdate(udi, sig);
    PyMem_Free(udi);
    PyMem_Free(sig);
    return Py_BuildValue("i", result);
}

static PyObject *
Db_addOrUpdate(recollq_DbObject* self, PyObject *args, PyObject *)
{
    LOGDEB(("Db_addOrUpdate\n"));
    char *udi = 0;
    char *parent_udi = 0;

    recollq_DocObject *pydoc;

    if (!PyArg_ParseTuple(args, "esesO!:Db_makeDocAbstract",
			  "utf-8", &udi, "utf-8", &parent_udi, 
			  &recollq_DocType, &pydoc)) {
	return 0;
    }
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGERR(("Db_addOrUpdate: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db");
        return 0;
    }
    if (pydoc->doc == 0 || the_docs.find(pydoc->doc) == the_docs.end()) {
	LOGERR(("Db_addOrUpdate: doc not found %p\n", pydoc->doc));
        PyErr_SetString(PyExc_AttributeError, "doc");
        return 0;
    }
    if (!self->db->addOrUpdate(udi, parent_udi, *pydoc->doc)) {
	LOGERR(("Db_addOrUpdate: rcldb error\n"));
        PyErr_SetString(PyExc_AttributeError, "rcldb error");
	PyMem_Free(udi);
	PyMem_Free(parent_udi);
        return 0;
    }
    PyMem_Free(udi);
    PyMem_Free(parent_udi);
    Py_RETURN_NONE;
}
    
static PyMethodDef Db_methods[] = {
    {"query", (PyCFunction)Db_query, METH_NOARGS,
     "Return a new, blank query for this index"
    },
    {"setAbstractParams", (PyCFunction)Db_setAbstractParams, 
     METH_VARARGS|METH_KEYWORDS,
     "Set abstract build parameters: maxchars and contextwords"
    },
    {"makeDocAbstract", (PyCFunction)Db_makeDocAbstract, METH_VARARGS,
     "Build keyword in context abstract for document and query"
    },
    {"needUpdate", (PyCFunction)Db_needUpdate, METH_VARARGS,
     "Check index up to date"
    },
    {"addOrUpdate", (PyCFunction)Db_addOrUpdate, METH_VARARGS,
     "Add or update document in index"
    },
    {NULL}  /* Sentinel */
};

/////////////////////////////////////////////
/// Query object method
static void 
Query_dealloc(recollq_QueryObject *self)
{
    LOGDEB(("Query_dealloc\n"));
    if (self->query)
	the_queries.erase(self->query);
    delete self->query;
    self->query = 0;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Query_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    recollq_QueryObject *self;
    LOGDEB(("Query_new\n"));

    self = (recollq_QueryObject *)type->tp_alloc(type, 0);
    if (self == 0) 
	return 0;
    self->query = 0;
    self->next = -1;
    return (PyObject *)self;
}

// Query_init creates an unusable object. The only way to create a
// valid Query Object is through db_query(). (or we'd need to add a Db
// parameter to the Query object creation method)
static int
Query_init(recollq_QueryObject *self, PyObject *, PyObject *)
{
    LOGDEB(("Query_init\n"));

    if (self->query)
	the_queries.erase(self->query);
    delete self->query;
    self->query = 0;
    self->next = -1;
    return 0;
}

static PyObject *
Query_execute(recollq_QueryObject* self, PyObject *args, PyObject *kwds)
{
    char *utf8 = 0;
    LOGDEB(("Query_execute\n"));
    if (!PyArg_ParseTuple(args, "es:Query_execute", "utf-8", &utf8)) {
	return 0;
    }

    LOGDEB(("Query_execute:  [%s]\n", utf8));
    if (self->query == 0 || 
	the_queries.find(self->query) == the_queries.end()) {
        PyErr_SetString(PyExc_AttributeError, "query");
	return 0;
    }
    string reason;
    Rcl::SearchData *sd = wasaStringToRcl(utf8, reason);
    PyMem_Free(utf8);
    if (!sd) {
	PyErr_SetString(PyExc_ValueError, reason.c_str());
	return 0;
    }
    RefCntr<Rcl::SearchData> rq(sd);
    self->query->setQuery(rq, Rcl::Query::QO_STEM);
    int cnt = self->query->getResCnt();
    self->next = 0;
    return Py_BuildValue("i", cnt);
}

static PyObject *
Query_fetchone(recollq_QueryObject* self, PyObject *, PyObject *)
{
    LOGDEB(("Query_fetchone\n"));

    if (self->query == 0 || 
	the_queries.find(self->query) == the_queries.end()) {
        PyErr_SetString(PyExc_AttributeError, "query");
	return 0;
    }
    int cnt = self->query->getResCnt();
    if (cnt <= 0 || self->next < 0) {
        PyErr_SetString(PyExc_AttributeError, "query: no results");
	return 0;
    }
    recollq_DocObject *result = 
	(recollq_DocObject *)obj_Create(&recollq_DocType, 0, 0);
    if (!result) {
	LOGERR(("Query_fetchone: couldn't create doc object for result\n"));
	return 0;
    }
    int percent;
    if (!self->query->getDoc(self->next, *result->doc, &percent)) {
        PyErr_SetString(PyExc_EnvironmentError, "query: cant fetch result");
	self->next = -1;
	return 0;
    }
    self->next++;
    // Move some data from the dedicated fields to the meta array to make 
    // fetching attributes easier
    Rcl::Doc *doc = result->doc;
    printableUrl(rclconfig->getDefCharset(), doc->url, doc->meta["url"]);
    doc->meta["mimetype"] = doc->mimetype;
    doc->meta["mtime"] = doc->dmtime.empty() ? doc->fmtime : doc->dmtime;
    doc->meta["ipath"] = doc->ipath;
    doc->meta["fbytes"] = doc->fbytes;
    doc->meta["dbytes"] = doc->dbytes;
    char pc[20];
    sprintf(pc, "%02d %%", percent);
    doc->meta["relevance"] = pc;

    LOGDEB(("Query_fetchone: done\n"));
    return (PyObject *)result;
}

static PyMethodDef Query_methods[] = {
    {"execute", 
     (PyCFunction)Query_execute, 
     METH_VARARGS,
     "Execute a search"
    },
    {"fetchone", 
     (PyCFunction)Query_fetchone, 
     METH_VARARGS,
     "Fetch result at rank i"
    },
    {NULL}  /* Sentinel */
};
static PyMemberDef Query_members[] = {
    {"next", T_INT, offsetof(recollq_QueryObject, next), 0,
     "Next index to be fetched from query results"},
    {NULL}  /* Sentinel */
};

///////////////////////////////////////////////////////////////////////
///// Doc object methods
static void 
Doc_dealloc(recollq_DocObject *self)
{
    LOGDEB(("Doc_dealloc\n"));
    if (self->doc)
	the_docs.erase(self->doc);
    delete self->doc;
    self->doc = 0;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Doc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    recollq_DocObject *self;
    LOGDEB(("Doc_new\n"));

    self = (recollq_DocObject *)type->tp_alloc(type, 0);
    if (self == 0) 
	return 0;
    self->doc = 0;
    return (PyObject *)self;
}

static int
Doc_init(recollq_DocObject *self, PyObject *, PyObject *)
{
    LOGDEB(("Doc_init\n"));
    if (self->doc)
	the_docs.erase(self->doc);
    delete self->doc;
    self->doc = new Rcl::Doc;
    if (self->doc == 0)
	return -1;
    the_docs.insert(self->doc);
    return 0;
}

// The "closure" thing is actually the meta field name. This is how
// python allows one set of get/set functions to get/set different
// attributes (pass them an additional parameters as from the
// getseters table and call it a "closure"
static PyObject *
Doc_getmeta(recollq_DocObject *self, void *closure)
{
    LOGDEB(("Doc_getmeta: [%s]\n", (const char *)closure));
    if (self->doc == 0 || 
	the_docs.find(self->doc) == the_docs.end()) {
        PyErr_SetString(PyExc_AttributeError, "doc");
	return 0;
    }
    LOGDEB(("Doc_getmeta: doc %p\n", self->doc));

#if 0
    for (map<string,string>::const_iterator it = self->doc->meta.begin();
	 it != self->doc->meta.end(); it++) {
	LOGDEB(("meta[%s] -> [%s]\n", it->first.c_str(), it->second.c_str()));
    }
#endif

    // Retrieve utf-8 coded value for meta field (if it doesnt exist,
    // this inserts a null value in the array, we could be nicer.
    string meta = self->doc->meta[(const char *)closure];
    // Return a python unicode object
    PyObject* res = PyUnicode_Decode(meta.c_str(), meta.size(), "UTF-8", 
				     "replace");
    return res;
}

static int
Doc_setmeta(recollq_DocObject *self, PyObject *value, void *closure)
{
    if (self->doc == 0 || 
	the_docs.find(self->doc) == the_docs.end()) {
        PyErr_SetString(PyExc_AttributeError, "doc??");
	return -1;
    }
    LOGDEB2(("Doc_setmeta: doc %p\n", self->doc));
    if (PyString_Check(value)) {
	value = PyUnicode_FromObject(value);
	if (value == 0) 
	    return -1;
    } 

    if (!PyUnicode_Check(value)) {
	PyErr_SetString(PyExc_AttributeError, "value not str/unicode??");
	return -1;
    }

    PyObject* putf8 = PyUnicode_AsUTF8String(value);
    if (putf8 == 0) {
	LOGERR(("Doc_setmeta: encoding to utf8 failed\n"));
	PyErr_SetString(PyExc_AttributeError, "value??");
	return -1;
    }

    char* uvalue = PyString_AsString(putf8);
    const char *key = (const char *)closure;
    if (key == 0) {
        PyErr_SetString(PyExc_AttributeError, "key??");
	return -1;
    }

    LOGDEB(("Doc_setmeta: setting [%s] to [%s]\n", key, uvalue));
    self->doc->meta[key] = uvalue;
    switch (key[0]) {
    case 'd':
	if (!strcmp(key, "dbytes")) {
	    self->doc->dbytes = uvalue;
	}
	break;
    case 'f':
	if (!strcmp(key, "fbytes")) {
	    self->doc->fbytes = uvalue;
	}
	break;
    case 'i':
	if (!strcmp(key, "ipath")) {
	    self->doc->ipath = uvalue;
	}
	break;
    case 'm':
	if (!strcmp(key, "mimetype")) {
	    self->doc->mimetype = uvalue;
	} else if (!strcmp(key, "mtime")) {
	    self->doc->dmtime = uvalue;
	}
	break;
    case 's':
	if (!strcmp(key, "sig")) {
	    self->doc->sig = uvalue;
	}
	break;
    case 't':
	if (!strcmp(key, "text")) {
	    self->doc->text = uvalue;
	}
	break;
    case 'u':
	if (!strcmp(key, "url")) {
	    self->doc->url = uvalue;
	}
	break;
    }
    return 0;
}

static PyGetSetDef Doc_getseters[] = {
    // Name, get, set, doc, closure
    {"url", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "url", (void *)"url"},
    {"ipath", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "ipath", (void *)"ipath"},
    {"mimetype", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "mimetype", (void *)"mimetype"},
    {"mtime", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "mtime", (void *)"mtime"},
    {"fbytes", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "fbytes", (void *)"fbytes"},
    {"dbytes", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "dbytes", (void *)"dbytes"},
    {"relevance", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "relevance", (void *)"relevance"},
    {"title", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "title", (void *)"title"},
    {"keywords", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "keywords", (void *)"keywords"},
    {"abstract", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "abstract", (void *)"abstract"},
    {"author", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "author", (void *)"author"},
    {"text", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "text", (void *)"text"},
    {"sig", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "sig", (void *)"sig"},
    {NULL}  /* Sentinel */
};


//////////////////////////////////////////////////////////////////////////
// Module methods
static PyObject *
recollq_connect(PyObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("recollq_connect\n"));
    recollq_DbObject *db = 
	(recollq_DbObject *)obj_Create(&recollq_DbType, args, kwargs);

    return (PyObject *)db;
}

static PyMethodDef recollqMethods[] = {
    {"connect",  (PyCFunction)recollq_connect, METH_VARARGS|METH_KEYWORDS, 
     "Connect to index and get Db object."},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};


RclConfig *RclConfig::getMainConfig() 
{
    return rclconfig;
}

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initrecollq(void)
{
    string reason;
    rclconfig = recollinit(0, 0, reason, 0);
    if (rclconfig == 0) {
	PyErr_SetString(PyExc_EnvironmentError, reason.c_str());
	return;
    }
    if (!rclconfig->ok()) {
	PyErr_SetString(PyExc_EnvironmentError, 
			"Recoll init error: bad environment ?");
	return;
    }

    PyObject* m;
    m = Py_InitModule3("recollq", recollqMethods,
                       "Recoll query extension module.");
    
    recollq_DbType.tp_dealloc = (destructor)Db_dealloc;
    recollq_DbType.tp_new = Db_new;
    recollq_DbType.tp_init = (initproc)Db_init;
    recollq_DbType.tp_methods = Db_methods;
    if (PyType_Ready(&recollq_DbType) < 0)
        return;
    LOGDEB(("initrecollq: recollq_DbType new %p\n", recollq_DbType.tp_new));
    Py_INCREF(&recollq_DbType);
    PyModule_AddObject(m, "Db", (PyObject *)&recollq_DbType);

    recollq_QueryType.tp_dealloc = (destructor)Query_dealloc;
    recollq_QueryType.tp_new = Query_new;
    recollq_QueryType.tp_init = (initproc)Query_init;
    recollq_QueryType.tp_methods = Query_methods;
    recollq_QueryType.tp_members = Query_members;
    if (PyType_Ready(&recollq_QueryType) < 0)
        return;
    Py_INCREF(&recollq_QueryType);
    PyModule_AddObject(m, "Query", (PyObject *)&recollq_QueryType);

    recollq_DocType.tp_dealloc = (destructor)Doc_dealloc;
    recollq_DocType.tp_new = Doc_new;
    recollq_DocType.tp_init = (initproc)Doc_init;
    recollq_DocType.tp_getset = Doc_getseters;
    //    recollq_DocType.tp_methods = Doc_methods;
    if (PyType_Ready(&recollq_DocType) < 0)
        return;
    Py_INCREF(&recollq_DocType);
    PyModule_AddObject(m, "Doc", (PyObject *)&recollq_DocType);

    LOGDEB(("initrecollq ok\n"));
}
