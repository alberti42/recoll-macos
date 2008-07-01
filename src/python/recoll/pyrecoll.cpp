#ifndef lint
static char rcsid[] = "@(#$Id: pyrecoll.cpp,v 1.5 2008-07-01 08:24:30 dockes Exp $ (C) 2007 J.F.Dockes";
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
    0,    /*tp_dealloc*/
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
    "Recollq Db objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
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
    "Recollq Query objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
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
    "Recollq Doc objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

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
    static char *kwlist[] = {"confdir", "extra_dbs", NULL};
    PyObject *extradbs = 0;
    char *confdir = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sO", kwlist,
				     &confdir, &extradbs))
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
    if (!self->db->open(dbdir, rclconfig->getStopfile(), Rcl::Db::DbRO)) {
	LOGDEB(("Db_init: db open error\n"));
	PyErr_SetString(PyExc_EnvironmentError, "Cant open index");
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

static PyMethodDef Db_methods[] = {
    {"query", (PyCFunction)Db_query, METH_NOARGS,
     "Return a new, blank query for this index"
    },
    {"setAbstractParams", (PyCFunction)Db_setAbstractParams, 
     METH_VARARGS|METH_KEYWORDS,
     "Set abstract build params: maxchars and contextwords"
    },
    {"makeDocAbstract", (PyCFunction)Db_makeDocAbstract, METH_VARARGS,
     "Return a new, blank query for this index"
    },
    {NULL}  /* Sentinel */
};

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
    int len = 0;
    LOGDEB(("Query_execute\n"));
    if (!PyArg_ParseTuple(args, "es#:Query_execute", "utf-8", &utf8, &len)) {
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
    Rcl::Doc *doc = new Rcl::Doc;
    int percent;
    if (!self->query->getDoc(self->next, *doc, &percent)) {
        PyErr_SetString(PyExc_EnvironmentError, "query: cant fetch result");
	self->next = -1;
	return 0;
    }
    self->next++;
    recollq_DocObject *result = 
	(recollq_DocObject *)obj_Create(&recollq_DocType, 0, 0);
    if (!result) {
	delete doc;
	return 0;
    }
    result->doc = doc;
    the_docs.insert(result->doc);
    // Move some data from the dedicated fields to the meta array to make 
    // fetching attributes easier
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
    self->doc = 0;
    return 0;
}

static PyObject *
Doc_getmeta(recollq_DocObject *self, void *closure)
{
    LOGDEB(("Doc_getmeta\n"));
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
    PyErr_SetString(PyExc_RuntimeError, "Cannot set attributes for now");
    return -1;
}

static PyGetSetDef Doc_getseters[] = {
    // Name, get, set, doc, closure
    {"title", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "title", (void *)"title"},
    {"keywords", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "keywords", (void *)"keywords"},
    {"abstract", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "abstract", (void *)"abstract"},
    {"url", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "url", (void *)"url"},
    {"mimetype", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "mimetype", (void *)"mimetype"},
    {"mtime", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "mtime", (void *)"mtime"},
    {"ipath", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "ipath", (void *)"ipath"},
    {"fbytes", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "fbytes", (void *)"fbytes"},
    {"dbytes", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "dbytes", (void *)"dbytes"},
    {"relevance", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "relevance", (void *)"relevance"},
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
