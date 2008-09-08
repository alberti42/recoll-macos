#ifndef lint
static char rcsid[] = "@(#$Id: pyrecoll.cpp,v 1.11 2008-09-08 16:49:10 dockes Exp $ (C) 2007 J.F.Dockes";
#endif


#include <Python.h>
#include <structmember.h>

#include <strings.h>

#include <string>
#include <iostream>
#include <set>
using namespace std;

#include "rclinit.h"
#include "rclconfig.h"
#include "rcldb.h"
#include "searchdata.h"
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

//////////////////////////////////////////////////////////////////////
/// SearchData code
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    RefCntr<Rcl::SearchData> sd;
} recoll_SearchDataObject;

static void 
SearchData_dealloc(recoll_SearchDataObject *self)
{
    LOGDEB(("SearchData_dealloc\n"));
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
SearchData_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LOGDEB(("SearchData_new\n"));
    recoll_SearchDataObject *self;

    self = (recoll_SearchDataObject *)type->tp_alloc(type, 0);
    if (self == 0) 
	return 0;
    return (PyObject *)self;
}

static int
SearchData_init(recoll_SearchDataObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("SearchData_init\n"));
    static char *kwlist[] = {"type", NULL};
    char *stp = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", kwlist, &stp))
	return -1;
    Rcl::SClType tp = Rcl::SCLT_AND;

    if (stp && strcasecmp(stp, "or")) {
	tp = Rcl::SCLT_OR;
    }
    self->sd = RefCntr<Rcl::SearchData>(new Rcl::SearchData(tp));
    return 0;
}

PyDoc_STRVAR(doc_addClause,
"addClause(type='and'|'or'|'excl'|'phrase'|'near'|'sub', qstring=string,\n"
"          slack=int, field=string, subSearch=SearchData,\n"
"Adds a simple clause to the SearchData And/Or chain, or a subquery\n"
"defined by another SearchData object\n"
);
/* Note: necessite And/Or vient du fait que le string peut avoir
   plusieurs mots. A transferer dans l'i/f Python ou pas ? */

/* Forward decl, def needs recoll_searchDataTyep */
static PyObject *
SearchData_addClause(recoll_SearchDataObject* self, PyObject *args, 
		     PyObject *kwargs);

static PyMethodDef SearchData_methods[] = {
    {"addClause", (PyCFunction)SearchData_addClause, METH_VARARGS|METH_KEYWORDS,
     doc_addClause
    },
    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(doc_SearchDataObject,
"SearchData()\n"
"\n"
"A SearchData object describes a query.\n"
);
static PyTypeObject recoll_SearchDataType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recoll.SearchData",             /*tp_name*/
    sizeof(recoll_SearchDataObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SearchData_dealloc,    /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    doc_SearchDataObject,      /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SearchData_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)SearchData_init, /* tp_init */
    0,                         /* tp_alloc */
    SearchData_new,            /* tp_new */
};

static PyObject *
SearchData_addClause(recoll_SearchDataObject* self, PyObject *args, 
		     PyObject *kwargs)
{
    LOGDEB(("SearchData_addClause\n"));
    if (self->sd.isNull()) {
	LOGERR(("SearchData_addClause: not init??\n"));
        PyErr_SetString(PyExc_AttributeError, "sd");
        return 0;
    }
    static char *kwlist[] = {"type", "qstring", "slack", "field",
			     "subsearch", NULL};
    char *tp = 0;
    char *qs = 0;
    int slack = 0;
    char *fld = 0;
    recoll_SearchDataObject *sub = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ses|iesO!", kwlist,
				     &tp, "utf-8", &qs, &slack,
				     "utf-8", &fld, 
				     &recoll_SearchDataType, &sub))
	return 0;

    Rcl::SearchDataClause *cl = 0;

    switch (tp[0]) {
    case 'a':
    case 'A':
	if (strcasecmp(tp, "and"))
	    goto defaultcase;
	cl = new Rcl::SearchDataClauseSimple(Rcl::SCLT_AND, qs, fld?fld:"");
	break;
    case 'o':
    case 'O':
	if (strcasecmp(tp, "or"))
	    goto defaultcase;
	cl = new Rcl::SearchDataClauseSimple(Rcl::SCLT_OR, qs, fld?fld:"");
	break;
    case 'e':
    case 'E':
	if (strcasecmp(tp, "excl"))
	    goto defaultcase;
	cl = new Rcl::SearchDataClauseSimple(Rcl::SCLT_EXCL, qs, fld?fld:"");
	break;
    case 'n':
    case 'N':
	if (strcasecmp(tp, "near"))
	    goto defaultcase;
	cl = new Rcl::SearchDataClauseDist(Rcl::SCLT_NEAR, qs, slack, 
					   fld ? fld : "");
	break;
    case 'p':
    case 'P':
	if (strcasecmp(tp, "phrase"))
	    goto defaultcase;
	cl = new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, qs, slack, 
					   fld ? fld : "");
	break;
    case 's':
    case 'S':
	if (strcasecmp(tp, "sub"))
	    goto defaultcase;
	cl = new Rcl::SearchDataClauseSub(Rcl::SCLT_SUB, sub->sd);
	break;
    defaultcase:
    default:
        PyErr_SetString(PyExc_AttributeError, "Bad tp arg");
	return 0;
    }

    self->sd->addClause(cl);
    Py_RETURN_NONE;
}

///////////////////////////////////////////////////////////////////////
///// Doc code
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Rcl::Doc *doc;
} recoll_DocObject;

static void 
Doc_dealloc(recoll_DocObject *self)
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
    recoll_DocObject *self;
    LOGDEB(("Doc_new\n"));

    self = (recoll_DocObject *)type->tp_alloc(type, 0);
    if (self == 0) 
	return 0;
    self->doc = 0;
    return (PyObject *)self;
}

static int
Doc_init(recoll_DocObject *self, PyObject *, PyObject *)
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
Doc_getmeta(recoll_DocObject *self, void *closure)
{
    LOGDEB0(("Doc_getmeta: [%s]\n", (const char *)closure));
    if (self->doc == 0 || 
	the_docs.find(self->doc) == the_docs.end()) {
        PyErr_SetString(PyExc_AttributeError, "doc");
	return 0;
    }

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
Doc_setmeta(recoll_DocObject *self, PyObject *value, void *closure)
{
    if (self->doc == 0 || 
	the_docs.find(self->doc) == the_docs.end()) {
        PyErr_SetString(PyExc_AttributeError, "doc??");
	return -1;
    }
    LOGDEB0(("Doc_setmeta: doc %p\n", self->doc));
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

    LOGDEB0(("Doc_setmeta: setting [%s] to [%s]\n", key, uvalue));
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
    {"relevancyrating", (getter)Doc_getmeta, (setter)Doc_setmeta, 
     "relevance", (void *)"relevancyrating"},
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

PyDoc_STRVAR(doc_DocObject,
"Doc()\n"
"\n"
"A Doc object contains index data for a given document.\n"
"The data is extracted from the index when searching, or set by the\n"
"indexer program when updating. See the data attributes list for more\n"
"details."
);
static PyTypeObject recoll_DocType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recoll.Doc",             /*tp_name*/
    sizeof(recoll_DocObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Doc_dealloc,    /*tp_dealloc*/
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
    doc_DocObject,             /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    Doc_getseters,             /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Doc_init,        /* tp_init */
    0,                         /* tp_alloc */
    Doc_new,                   /* tp_new */
};


//////////////////////////////////////////////////////


typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Rcl::Query *query;
    int         next; // Index of result to be fetched next or -1 if uninit
} recoll_QueryObject;
/////////////////////////////////////////////
/// Query object 
static void 
Query_dealloc(recoll_QueryObject *self)
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
    recoll_QueryObject *self;
    LOGDEB(("Query_new\n"));

    self = (recoll_QueryObject *)type->tp_alloc(type, 0);
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
Query_init(recoll_QueryObject *self, PyObject *, PyObject *)
{
    LOGDEB(("Query_init\n"));

    if (self->query)
	the_queries.erase(self->query);
    delete self->query;
    self->query = 0;
    self->next = -1;
    return 0;
}

PyDoc_STRVAR(doc_Query_execute,
"execute(query_string, stemmming=1|0)\n"
"\n"
"Starts a search for query_string, a Xesam user language string.\n"
"The query can be a simple list of terms (and'ed by default), or more\n"
"complicated with field specs etc. See the Recoll manual.\n"
);

static PyObject *
Query_execute(recoll_QueryObject* self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("Query_execute\n"));
    static char *kwlist[] = {"query_string", "stemming", NULL};
    char *utf8 = 0;
    int dostem = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "es|i:Query_execute", 
				     kwlist, "utf-8", &utf8,
				     &dostem)) {
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
    self->query->setQuery(rq, dostem?Rcl::Query::QO_STEM:Rcl::Query::QO_NONE);
    int cnt = self->query->getResCnt();
    self->next = 0;
    return Py_BuildValue("i", cnt);
}

PyDoc_STRVAR(doc_Query_executesd,
"execute(SearchData, stemming=1|0)\n"
"\n"
"Starts a search for the query defined by SearchData.\n"
);

static PyObject *
Query_executesd(recoll_QueryObject* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"searchdata", "stemming", NULL};
    recoll_SearchDataObject *pysd = 0;
    int dostem = 1;
    LOGDEB(("Query_executeSD\n"));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i:Query_execute", kwlist,
				     &recoll_SearchDataType, &pysd, &dostem)) {
	return 0;
    }
    if (self->query == 0 || 
	the_queries.find(self->query) == the_queries.end()) {
        PyErr_SetString(PyExc_AttributeError, "query");
	return 0;
    }
    self->query->setQuery(pysd->sd, dostem ? Rcl::Query::QO_STEM : 
			  Rcl::Query::QO_NONE);
    int cnt = self->query->getResCnt();
    self->next = 0;
    return Py_BuildValue("i", cnt);
}

PyDoc_STRVAR(doc_Query_fetchone,
"fetchone(None) -> Doc\n"
"\n"
"Fetches the next Doc object in the current search results.\n"
);
static PyObject *
Query_fetchone(recoll_QueryObject* self, PyObject *, PyObject *)
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
    recoll_DocObject *result = 
	(recoll_DocObject *)obj_Create(&recoll_DocType, 0, 0);
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

    return (PyObject *)result;
}

static PyMethodDef Query_methods[] = {
    {"execute", (PyCFunction)Query_execute, METH_VARARGS|METH_KEYWORDS, 
     doc_Query_execute},
    {"executesd", (PyCFunction)Query_executesd, METH_VARARGS|METH_KEYWORDS, 
     doc_Query_executesd},
    {"fetchone", (PyCFunction)Query_fetchone, METH_VARARGS,doc_Query_fetchone},
    {NULL}  /* Sentinel */
};

static PyMemberDef Query_members[] = {
    {"next", T_INT, offsetof(recoll_QueryObject, next), 0,
     "Next index to be fetched from results.\n"
     "Can be set/reset before calling fetchone() to effect seeking.\n"
     "Starts at 0"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject recoll_QueryType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recoll.Query",             /*tp_name*/
    sizeof(recoll_QueryObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Query_dealloc, /*tp_dealloc*/
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
    "Recoll Query object",    /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Query_methods,             /* tp_methods */
    Query_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Query_init,      /* tp_init */
    0,                         /* tp_alloc */
    Query_new,                 /* tp_new */
};


///////////////////////////////////////////////
////// Db object code
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Rcl::Db *db;
} recoll_DbObject;

static void 
Db_dealloc(recoll_DbObject *self)
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
    recoll_DbObject *self;

    self = (recoll_DbObject *)type->tp_alloc(type, 0);
    if (self == 0) 
	return 0;
    self->db = 0;
    return (PyObject *)self;
}

static int
Db_init(recoll_DbObject *self, PyObject *args, PyObject *kwargs)
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

    if (!self->db->open(dbdir, rclconfig->getStopfile(), writable ? 
			Rcl::Db::DbUpd : Rcl::Db::DbRO)) {
	LOGERR(("Db_init: db open error\n"));
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
    return 0;
}

static PyObject *
Db_query(recoll_DbObject* self)
{
    LOGDEB(("Db_query\n"));
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGERR(("Db_query: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db");
        return 0;
    }
    recoll_QueryObject *result = 
	(recoll_QueryObject *)obj_Create(&recoll_QueryType, 0, 0);
    if (!result)
	return 0;
    result->query = new Rcl::Query(self->db);
    the_queries.insert(result->query);
    return (PyObject *)result;
}

static PyObject *
Db_setAbstractParams(recoll_DbObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("Db_setAbstractParams\n"));
    static char *kwlist[] = {"maxchars", "contextwords", NULL};
    int ctxwords = -1, maxchars = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ii", kwlist,
				     &maxchars, &ctxwords))
	return 0;
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGERR(("Db_query: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db id not found");
        return 0;
    }
    self->db->setAbstractParams(-1, maxchars, ctxwords);
    Py_RETURN_NONE;
}

static PyObject *
Db_makeDocAbstract(recoll_DbObject* self, PyObject *args, PyObject *)
{
    LOGDEB(("Db_makeDocAbstract\n"));
    recoll_DocObject *pydoc = 0;
    recoll_QueryObject *pyquery = 0;
    if (!PyArg_ParseTuple(args, "O!O!:Db_makeDocAbstract",
			  &recoll_DocType, &pydoc,
			  &recoll_QueryType, &pyquery)) {
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
Db_needUpdate(recoll_DbObject* self, PyObject *args, PyObject *kwds)
{
    char *udi = 0;
    char *sig = 0;
    LOGDEB(("Db_needUpdate\n"));
    if (!PyArg_ParseTuple(args, "eses:Db_needUpdate", 
			  "utf-8", &udi, "utf-8", &sig)) {
	return 0;
    }
    if (self->db == 0 || the_dbs.find(self->db) == the_dbs.end()) {
	LOGERR(("Db_needUpdate: db not found %p\n", self->db));
        PyErr_SetString(PyExc_AttributeError, "db");
        return 0;
    }
    bool result = self->db->needUpdate(udi, sig);
    PyMem_Free(udi);
    PyMem_Free(sig);
    return Py_BuildValue("i", result);
}

static PyObject *
Db_addOrUpdate(recoll_DbObject* self, PyObject *args, PyObject *)
{
    LOGDEB(("Db_addOrUpdate\n"));
    char *udi = 0;
    char *parent_udi = 0;

    recoll_DocObject *pydoc;

    if (!PyArg_ParseTuple(args, "esO!|es:Db_addOrUpdate",
			  "utf-8", &udi, &recoll_DocType, &pydoc,
			  "utf-8", &parent_udi)) {
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
    if (!self->db->addOrUpdate(udi, parent_udi?parent_udi:"", *pydoc->doc)) {
	LOGERR(("Db_addOrUpdate: rcldb error\n"));
        PyErr_SetString(PyExc_AttributeError, "rcldb error");
	PyMem_Free(udi);
	PyMem_Free(parent_udi);
        return 0;
    }
    PyMem_Free(udi);
    if (parent_udi)
	PyMem_Free(parent_udi);
    Py_RETURN_NONE;
}
    
static PyMethodDef Db_methods[] = {
    {"query", (PyCFunction)Db_query, METH_NOARGS,
     "query() -> Query. Return a new, blank query object for this index."
    },
    {"setAbstractParams", (PyCFunction)Db_setAbstractParams, 
     METH_VARARGS|METH_KEYWORDS,
     "setAbstractParams(maxchars, contextword).\n"
     "Set the parameters used to build keyword in context abstracts"
    },
    {"makeDocAbstract", (PyCFunction)Db_makeDocAbstract, METH_VARARGS,
     "makeDocAbstract(Doc, Query) -> abstract string.\n"
     "Build and return keyword in context abstract."
    },
    {"needUpdate", (PyCFunction)Db_needUpdate, METH_VARARGS,
     "needUpdate(udi, sig) -> Bool.\n"
     "Check index up to date for doc udi having current signature sig."
    },
    {"addOrUpdate", (PyCFunction)Db_addOrUpdate, METH_VARARGS,
     "addOrUpdate(udi, doc, parent_udi=None)\n"
     "Add or update document doc having unique id udi\n"
     "If parent_udi is set, this is the udi for the\n"
     "container (ie mbox file)"
    },
    {NULL}  /* Sentinel */
};
PyDoc_STRVAR(doc_DbObject,
"Db([confdir=None], [extra_dbs=None], [writable = False])\n"
"\n"
"A Db object connects to a Recoll database.\n"
"confdir specifies a Recoll configuration directory (default: environment).\n"
"extra_dbs is a list of external databases (xapian directories)\n"
"writable decides if we can index new data through this connection\n"
);
static PyTypeObject recoll_DbType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recoll.Db",             /*tp_name*/
    sizeof(recoll_DbObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Db_dealloc,    /*tp_dealloc*/
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
    doc_DbObject,              /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Db_methods,                /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Db_init,         /* tp_init */
    0,                         /* tp_alloc */
    Db_new,                    /* tp_new */
};


//////////////////////////////////////////////////////////////////////////
// Module methods
static PyObject *
recoll_connect(PyObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("recoll_connect\n"));
    recoll_DbObject *db = 
	(recoll_DbObject *)obj_Create(&recoll_DbType, args, kwargs);

    return (PyObject *)db;
}

PyDoc_STRVAR(doc_connect,
"connect([confdir=None], [extra_dbs=None], [writable = False])\n"
"         -> Db.\n"
"\n"
"Connects to a Recoll database and returns a Db object.\n"
"confdir specifies a Recoll configuration directory (default: environment).\n"
"extra_dbs is a list of external databases (xapian directories)\n"
"writable decides if we can index new data through this connection\n"
);

static PyMethodDef recollMethods[] = {
    {"connect",  (PyCFunction)recoll_connect, METH_VARARGS|METH_KEYWORDS, 
     doc_connect},

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
initrecoll(void)
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
    m = Py_InitModule3("recoll", recollMethods,
                       "Recoll extension module.");
    
    if (PyType_Ready(&recoll_DbType) < 0)
        return;
    Py_INCREF(&recoll_DbType);
    PyModule_AddObject(m, "Db", (PyObject *)&recoll_DbType);

    if (PyType_Ready(&recoll_QueryType) < 0)
        return;
    Py_INCREF(&recoll_QueryType);
    PyModule_AddObject(m, "Query", (PyObject *)&recoll_QueryType);

    if (PyType_Ready(&recoll_DocType) < 0)
        return;
    Py_INCREF(&recoll_DocType);
    PyModule_AddObject(m, "Doc", (PyObject *)&recoll_DocType);

    if (PyType_Ready(&recoll_SearchDataType) < 0)
        return;
    Py_INCREF(&recoll_SearchDataType);
    PyModule_AddObject(m, "SearchData", (PyObject *)&recoll_SearchDataType);
}
