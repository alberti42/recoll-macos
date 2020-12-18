/* Copyright (C) 2007-2020 J.F.Dockes
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <Python.h>
#include <structmember.h>
#include <bytesobject.h>

#include <string>
#include <iostream>
#include <set>

#include "qresultstore.h"

#include "pyrecoll.h"
#include "log.h"

using namespace std;

#if PY_MAJOR_VERSION >=3
#  define Py_TPFLAGS_HAVE_ITER 0
#else
#define PyLong_FromLong PyInt_FromLong 
#endif

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Rcl::QResultStore *store;
} recoll_QResultStoreObject;

static void 
QResultStore_dealloc(recoll_QResultStoreObject *self)
{
    LOGDEB("QResultStore_dealloc.\n");
    delete self->store;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
QResultStore_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LOGDEB("QResultStore_new\n");
    recoll_QResultStoreObject *self;

    self = (recoll_QResultStoreObject *)type->tp_alloc(type, 0);
    if (self == 0) 
        return 0;
    self->store = new Rcl::QResultStore();
    return (PyObject *)self;
}

PyDoc_STRVAR(qrs_doc_QResultStoreObject,
             "QResultStore()\n"
             "\n"
             "A QResultStore can efficiently store query result documents.\n"
    );

static int
QResultStore_init(
    recoll_QResultStoreObject *self, PyObject *args, PyObject *kwargs)
{
    LOGDEB("QResultStore_init\n");
    return 0;
}

PyDoc_STRVAR(
    qrs_doc_storeQuery,
    "storeQuery(query, fieldspec=[], isinc=False)\n"
    "\n"
    "Stores the results from the input query object, possibly "
    "excluding/including the specified fields.\n"
    );

static PyObject *
QResultStore_storeQuery(recoll_QResultStoreObject* self, PyObject *args, 
                        PyObject *kwargs)
{
    static const char* kwlist[] = {"query", "fieldspec", "isinc", NULL};
    PyObject *q{nullptr};
    PyObject *fieldspec{nullptr};
    PyObject *isinco = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|OO", (char**)kwlist, 
                                     &recoll_QueryType, &q, &fieldspec, &isinco))
        return nullptr;

    recoll_QueryObject *query = (recoll_QueryObject*)q;
    if (nullptr == query->query) {
        PyErr_SetString(PyExc_ValueError,
                        "query not initialised (null query ?)");
        return nullptr;
    }
    bool isinc{false};
    if (nullptr != isinco && PyObject_IsTrue(isinco))
        isinc = true;
    
    std::set<std::string> fldspec;
    if (nullptr != fieldspec) {
        // fieldspec must be either single string or list of strings
        if (PyUnicode_Check(fieldspec)) {
            PyObject *utf8o = PyUnicode_AsUTF8String(fieldspec);
            if (nullptr == utf8o) {
                PyErr_SetString(PyExc_AttributeError,
                                "storeQuery: can't encode field name??");
                return nullptr;
            }
            fldspec.insert(PyBytes_AsString(utf8o));
            Py_DECREF(utf8o);
        } else if (PySequence_Check(fieldspec)) {
            for (Py_ssize_t i = 0; i < PySequence_Size(fieldspec); i++) {
                PyObject *utf8o =
                    PyUnicode_AsUTF8String(PySequence_GetItem(fieldspec, i));
                if (nullptr == utf8o) {
                    PyErr_SetString(PyExc_AttributeError,
                                    "storeQuery: can't encode field name??");
                    return nullptr;
                }
                fldspec.insert(PyBytes_AsString(utf8o));
                Py_DECREF(utf8o);
            }
        } else {
            PyErr_SetString(PyExc_TypeError,
                            "fieldspec arg must be str or sequence of str");
            return nullptr;
        }
    }
    self->store->storeQuery(*(query->query), fldspec, isinc);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    qrs_doc_getCount,
    "getCount()\n"
    "\n"
    "Return the stored results count.\n"
    );

static PyObject *
QResultStore_getCount(recoll_QResultStoreObject* self, PyObject *args)
{
    return PyLong_FromLong(self->store->getCount());
}


PyDoc_STRVAR(
    qrs_doc_getField,
    "getField(index, fieldname)\n"
    "\n"
    "Retrieve tha value of field <fieldname> from result at index <index>.\n"
    );

static PyObject *
QResultStore_getField(recoll_QResultStoreObject* self, PyObject *args)
{
    int index;
    const char *fieldname;
    if (!PyArg_ParseTuple(args, "is", &index, &fieldname)) {
        return nullptr;
    }
    const char *result = self->store->fieldValue(index, fieldname);
    if (nullptr == result) {
        Py_RETURN_NONE;
    } else {
        return PyBytes_FromString(result);
    }
}

static PyMethodDef QResultStore_methods[] = {
    {"storeQuery", (PyCFunction)QResultStore_storeQuery,
     METH_VARARGS|METH_KEYWORDS, qrs_doc_getCount},
    {"getCount", (PyCFunction)QResultStore_getCount,
     METH_VARARGS|METH_KEYWORDS, qrs_doc_storeQuery},
    {"getField", (PyCFunction)QResultStore_getField,
     METH_VARARGS, qrs_doc_getField},

    {NULL}  /* Sentinel */
};

PyTypeObject recoll_QResultStoreType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_recoll.QResultStore",             /*tp_name*/
    sizeof(recoll_QResultStoreObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)QResultStore_dealloc,    /*tp_dealloc*/
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
    qrs_doc_QResultStoreObject,      /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    QResultStore_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)QResultStore_init, /* tp_init */
    0,                         /* tp_alloc */
    QResultStore_new,            /* tp_new */
};

////////////////////////////////////////////////////////////////////////
// QRSDoc iterator
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    recoll_QResultStoreObject *pystore;
    int index;
} recoll_QRSDocObject;

static void 
QRSDoc_dealloc(recoll_QRSDocObject *self)
{
    Py_DECREF(self->pystore);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
QRSDoc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    recoll_QRSDocObject *self = (recoll_QRSDocObject *)type->tp_alloc(type, 0);
    if (self == 0) 
        return 0;
    return (PyObject *)self;
}

PyDoc_STRVAR(qrs_doc_QRSDocObject,
             "QRSDoc(resultstore, index)\n"
             "\n"
             "A QRSDoc gives access to one result from a qresultstore.\n"
    );

static int
QRSDoc_init(
    recoll_QRSDocObject *self, PyObject *args, PyObject *kwargs)
{
    recoll_QResultStoreObject *pystore;
    int index;
    if (!PyArg_ParseTuple(args, "O!i",
                          &recoll_QResultStoreType, &pystore, &index)) {
        return -1;
    }

    Py_INCREF(pystore);
    self->pystore = pystore;
    self->index = index;
    return 0;
}

static PyObject *
QRSDoc_subscript(recoll_QRSDocObject *self, PyObject *key)
{
    if (self->pystore == 0) {
        PyErr_SetString(PyExc_AttributeError, "store??");
        return NULL;
    }
    string name;
    if (PyUnicode_Check(key)) {
        PyObject* utf8o = PyUnicode_AsUTF8String(key);
        if (utf8o == 0) {
            PyErr_SetString(PyExc_AttributeError, "name??");
            Py_RETURN_NONE;
        }
        name = PyBytes_AsString(utf8o);
        Py_DECREF(utf8o);
    }  else if (PyBytes_Check(key)) {
        name = PyBytes_AsString(key);
    } else {
        PyErr_SetString(PyExc_AttributeError, "key not unicode nor string??");
        Py_RETURN_NONE;
    }
    const char *value = self->pystore->store->fieldValue(self->index, name);
    if (nullptr == value) {
        Py_RETURN_NONE;
    }
    return PyBytes_FromString(value);
}

static PyMappingMethods qrsdoc_as_mapping = {
    (lenfunc)0, /*mp_length*/
    (binaryfunc)QRSDoc_subscript, /*mp_subscript*/
    (objobjargproc)0, /*mp_ass_subscript*/
};

static PyMethodDef QRSDoc_methods[] = {
    {NULL}  /* Sentinel */
};


PyTypeObject recoll_QRSDocType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_recoll.QRSDoc",             /*tp_name*/
    sizeof(recoll_QRSDocObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)QRSDoc_dealloc,    /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    &qrsdoc_as_mapping,         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    qrs_doc_QRSDocObject,      /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    QRSDoc_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)QRSDoc_init, /* tp_init */
    0,                         /* tp_alloc */
    QRSDoc_new,            /* tp_new */
};
