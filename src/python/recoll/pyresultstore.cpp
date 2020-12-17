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


//////////////////////////////////////////////////////////////////////////
// Module methods
static PyMethodDef rclrstore_methods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


PyDoc_STRVAR(pyrclrstore_doc_string,
             "Utility module for efficiently storing many query results.\n");

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

#if PY_MAJOR_VERSION >= 3
static int rclrstore_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int rclrstore_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_rclrstore",
    NULL,
    sizeof(struct module_state),
    rclrstore_methods,
    NULL,
    rclrstore_traverse,
    rclrstore_clear,
    NULL
};

#define INITERROR return NULL
extern "C" PyObject *
PyInit__rclrstore(void)
#else
#define INITERROR return
    PyMODINIT_FUNC
    init__rclrstore(void)
#endif
{
    // Note: we can't call recollinit here, because the confdir is only really
    // known when the first db object is created (it is an optional parameter).
    // Using a default here may end up with variables such as stripchars being
    // wrong

#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("_rclrstore", rclrstore_methods);
#endif
    if (module == NULL)
        INITERROR;

    struct module_state *st = GETSTATE(module);
    // The first parameter is a char *. Hopefully we don't initialize
    // modules too often...
    st->error = PyErr_NewException(strdup("_rclrstore.Error"), NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }
    
    if (PyType_Ready(&recoll_QResultStoreType) < 0)
        INITERROR;
    Py_INCREF((PyObject*)&recoll_QResultStoreType);
    PyModule_AddObject(module, "QResultStore", 
                       (PyObject *)&recoll_QResultStoreType);

    PyModule_AddStringConstant(module, "__doc__", pyrclrstore_doc_string);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
