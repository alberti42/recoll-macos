/* Copyright (C) 2007 J.F.Dockes
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


#include <Python.h>
#include <structmember.h>
#include <bytearrayobject.h>

#include <strings.h>

#include <string>
using namespace std;

#include "debuglog.h"
#include "rcldoc.h"
#include "internfile.h"
#include "rclconfig.h"
#include "rclinit.h"

#include "pyrecoll.h"

// Imported from pyrecoll
static PyObject *recoll_DocType;
static RclConfig *rclconfig;

//////////////////////////////////////////////////////////////////////
/// Extractor object code
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    FileInterner *xtr;
    TempDir *tmpdir;
    RclConfig *rclconfig;
} rclx_ExtractorObject;

static void 
Extractor_dealloc(rclx_ExtractorObject *self)
{
    LOGDEB(("Extractor_dealloc\n"));
    delete self->xtr;
    delete self->tmpdir;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Extractor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LOGDEB(("Extractor_new\n"));
    rclx_ExtractorObject *self = 
	(rclx_ExtractorObject *)type->tp_alloc(type, 0);
    if (self == 0) 
	return 0;
    self->xtr = 0;
    self->tmpdir = 0;
    self->rclconfig = 0;
    return (PyObject *)self;
}

static int
Extractor_init(rclx_ExtractorObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB(("Extractor_init\n"));
    static const char* kwlist[] = {"doc", NULL};
    PyObject *pdobj;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", (char**)kwlist, 
				     recoll_DocType, &pdobj))
	return -1;
    recoll_DocObject *dobj = (recoll_DocObject *)pdobj;
    self->tmpdir = new TempDir;
    if (dobj->doc == 0) {
        PyErr_SetString(PyExc_AttributeError, "Null Doc ?");
	return -1;
    }
    self->rclconfig = dobj->rclconfig;
    self->xtr = new FileInterner(*dobj->doc, self->rclconfig, *self->tmpdir,
				 FileInterner::FIF_forPreview);
    return 0;
}

PyDoc_STRVAR(doc_Extractor_textextract,
"textextract(ipath)\n"
"Extract document defined by ipath and return a doc object. The doc.text\n"
"field has the document text as either text/plain or text/html\n"
"according to doc.mimetype.\n"
);

static PyObject *
Extractor_textextract(rclx_ExtractorObject* self, PyObject *args, 
		      PyObject *kwargs)
{
    LOGDEB(("Extractor_textextract\n"));
    static const char* kwlist[] = {"ipath", NULL};
    char *sipath = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "es:Extractor_textextract",
				     (char**)kwlist, 
				     "utf-8", &sipath))
	return 0;

    string ipath(sipath);
    PyMem_Free(sipath);

    if (self->xtr == 0) {
        PyErr_SetString(PyExc_AttributeError, "extract: null object");
	return 0;
    }
    /* Call the doc class object to create a new doc. */
    recoll_DocObject *result = 
       (recoll_DocObject *)PyObject_CallObject((PyObject *)recoll_DocType, 0);
    if (!result) {
        PyErr_SetString(PyExc_AttributeError, "extract: doc create failed");
	return 0;
    }
    FileInterner::Status status = self->xtr->internfile(*(result->doc), ipath);
    if (status != FileInterner::FIDone) {
        PyErr_SetString(PyExc_AttributeError, "internfile failure");
        return 0;
    }

    string html = self->xtr->get_html();
    if (!html.empty()) {
	result->doc->text = html;
	result->doc->mimetype = "text/html";
    }

    // Is this actually needed ? Useful for url which is also formatted .
    Rcl::Doc *doc = result->doc;
    printableUrl(self->rclconfig->getDefCharset(), doc->url, 
		 doc->meta[Rcl::Doc::keyurl]);
    doc->meta[Rcl::Doc::keytp] = doc->mimetype;
    doc->meta[Rcl::Doc::keyipt] = doc->ipath;
    doc->meta[Rcl::Doc::keyfs] = doc->fbytes;
    doc->meta[Rcl::Doc::keyds] = doc->dbytes;
    return (PyObject *)result;
}

PyDoc_STRVAR(doc_Extractor_idoctofile,
"idoctofile(ipath)\n"
"Extract document defined by ipath into a file, in its native format.\n"
);
static PyObject *
Extractor_idoctofile(rclx_ExtractorObject* self, PyObject *args, 
		     PyObject *kwargs)
{
    LOGDEB(("Extractor_idoctofile\n"));
    static const char* kwlist[] = {"ipath", "mimetype", "ofilename", NULL};
    char *sipath = 0;
    char *smt = 0;
    char *soutfile = 0; // no freeing
    if (!PyArg_ParseTupleAndKeywords(args,kwargs, "eses|s:Extractor_idoctofile",
				     (char**)kwlist, 
				     "utf-8", &sipath,
				     "utf-8", &smt,
				     &soutfile))
	return 0;

    string ipath(sipath);
    PyMem_Free(sipath);
    string mimetype(smt);
    PyMem_Free(smt);
    string outfile;
    if (soutfile && *soutfile)
	outfile.assign(soutfile); 
    
    if (self->xtr == 0) {
        PyErr_SetString(PyExc_AttributeError, "idoctofile: null object");
	return 0;
    }
    if (ipath.empty()) {
        PyErr_SetString(PyExc_ValueError, "idoctofile: null ipath");
	return 0;
    }
	
    self->xtr->setTargetMType(mimetype);
    TempFile temp;
    bool status = self->xtr->interntofile(temp, outfile, ipath, mimetype);
    if (!status) {
        PyErr_SetString(PyExc_AttributeError, "interntofile failure");
	return 0;
    }
    if (outfile.empty())
	temp->setnoremove(1);
    PyObject *result = outfile.empty() ? PyString_FromString(temp->filename()) :
	PyString_FromString(outfile.c_str());
    return (PyObject *)result;
}

static PyMethodDef Extractor_methods[] = {
    {"textextract", (PyCFunction)Extractor_textextract, 
     METH_VARARGS|METH_KEYWORDS, doc_Extractor_textextract},
    {"idoctofile", (PyCFunction)Extractor_idoctofile, 
     METH_VARARGS|METH_KEYWORDS, doc_Extractor_idoctofile},
    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(doc_ExtractorObject,
"Extractor()\n"
"\n"
"An Extractor object can extract data from a native simple or compound\n"
"object.\n"
);
static PyTypeObject rclx_ExtractorType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "rclextract.Extractor",             /*tp_name*/
    sizeof(rclx_ExtractorObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Extractor_dealloc,    /*tp_dealloc*/
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
    doc_ExtractorObject,      /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Extractor_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Extractor_init, /* tp_init */
    0,                         /* tp_alloc */
    Extractor_new,            /* tp_new */
};

///////////////////////////////////// Module-level stuff
static PyMethodDef rclxMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};
PyDoc_STRVAR(rclx_doc_string,
	     "This is an interface to the Recoll text extraction features.");

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initrclextract(void)
{
    // We run recollinit. It's responsible for initializing some static data
    // which is distinct from pyrecoll's as we're separately dlopened
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

    PyObject* m = Py_InitModule("rclextract", rclxMethods);
    PyModule_AddStringConstant(m, "__doc__", rclx_doc_string);

    if (PyType_Ready(&rclx_ExtractorType) < 0)
        return;
    Py_INCREF(&rclx_ExtractorType);
    PyModule_AddObject(m, "Extractor", (PyObject *)&rclx_ExtractorType);

    recoll_DocType = (PyObject*)PyCapsule_Import(PYRECOLL_PACKAGE "recoll.doctypeptr", 0);
}
