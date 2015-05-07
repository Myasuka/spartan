#include <Python.h>
#include <structmember.h>
#include <iostream>
#define NO_IMPORT_ARRAY
#include "cextent.h"

#define RETURN_IF_NULL(val) \
do {\
    if (val == NULL) { \
        return NULL; \
    } \
} while (0)

typedef struct {
    PyObject_HEAD
    int ndim;
    npy_intp size;
    CExtent *c_ex;
} TileExtent;

/* Can't be defined here because it need &TileExtentType. */
static PyObject* _TileExtent_create_helper(CExtent *c_ex, bool return_none_if_null);
static PyObject *mod = NULL;
static PyObject *create_obj = NULL;

static PyObject*
_TileExtent_gettuple(npy_intp *array, int ndim)
{
    PyObject *tuple = PyTuple_New(ndim);
    RETURN_IF_NULL(tuple);
    for (int i = 0; i < ndim; i++) {
        PyTuple_SET_ITEM(tuple, i, PyInt_FromLong(array[i]));
    }
    return tuple;
}

static int
_TileExtent_settuple(TileExtent *self,
                     PyObject *tuple,
                     npy_intp *dest,
                     bool init)
{
    int i;
    PyObject *o;

    if (!PyTuple_Check(tuple)) {
        PyErr_SetString(PyExc_TypeError, "value sould be a tuple");
        return -1;
    }

    for (i = 0; i < self->ndim; i++) {
        o = PyTuple_GET_ITEM(tuple, i);
        dest[i] = PyInt_AsLong(o);
    }

    if (init) {
        self->c_ex->init_info();
        self->size = self->c_ex->size;
    }

    return 0;
}

static PyObject*
TileExtent_getul(TileExtent *self, void *closure)
{
    return _TileExtent_gettuple(self->c_ex->ul, self->c_ex->ndim);
}

static int
TileExtent_setul(TileExtent *self, PyObject *value, void *closure)
{
    return _TileExtent_settuple(self, value, self->c_ex->ul, true);
}

static PyObject*
TileExtent_getlr(TileExtent *self, void *closure)
{
    return _TileExtent_gettuple(self->c_ex->lr, self->c_ex->ndim);
}

static int
TileExtent_setlr(TileExtent *self, PyObject *value, void *closure)
{
    return _TileExtent_settuple(self, value, self->c_ex->lr, true);
}

static PyObject*
TileExtent_getshape(TileExtent *self, void *closure)
{
    return _TileExtent_gettuple(self->c_ex->shape, self->c_ex->ndim);
}

static int
TileExtent_setshape(TileExtent *self, PyObject *value, void *closure)
{
    return -1;
}

static PyObject*
TileExtent_getarray_shape(TileExtent *self, void *closure)
{
    if (self->c_ex->has_array_shape) {
        return _TileExtent_gettuple(self->c_ex->array_shape, self->c_ex->ndim);
    } else {
        Py_RETURN_NONE;
    }
}

static int
TileExtent_setarray_shape(TileExtent *self, PyObject *value, void *closure)
{
    return _TileExtent_settuple(self, value, self->c_ex->array_shape, false);
}

static void
TileExtent_dealloc(PyObject *o)
{
    TileExtent *self = (TileExtent*) o;
    delete self->c_ex;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TileExtent_repr(PyObject *o)
{
    TileExtent *self = (TileExtent*) o;
    char s[2048] = "Extent: ul = (";
    char t1[1024] = "";
    char t2[1024] = "";

    for (int i = 0; i < self->ndim; ++i) {
        if (i != self->ndim - 1) {
            sprintf(t1, "%s%ld,", t1, self->c_ex->ul[i]);
            sprintf(t2, "%s%ld,", t2, self->c_ex->lr[i]);
        } else {
            sprintf(t1, "%s%ld", t1, self->c_ex->ul[i]);
            sprintf(t2, "%s%ld", t2, self->c_ex->lr[i]);
        }
    }
    strcat(s, t1);
    strcat(s, "), lr= (");
    strcat(s, t2);
    strcat(s, ")");

    return PyString_FromString(s);
}

static PyObject *
TileExtent_richcompare(PyObject* o, PyObject *o_other, int op)
{
    TileExtent *self = (TileExtent*) o;
    TileExtent *other = (TileExtent*) o_other;
    int ul_result, lr_result;

    ul_result = lr_result = 0;
    if (o_other == Py_None) {
        ul_result = lr_result = -1;
    } else {
        for (int i = 0; i < self->c_ex->ndim; i++) {
            if (self->c_ex->ul[i] > other->c_ex->ul[i])
                ul_result = 1;
            else if(self->c_ex->ul[i] < other->c_ex->ul[i])
                ul_result = -1;

            if (self->c_ex->lr[i] > other->c_ex->lr[i])
                lr_result = 1;
            else if(self->c_ex->lr[i] < other->c_ex->lr[i])
                lr_result = -1;
        }
    }
    switch (op) {
    case Py_GT:
        if (ul_result > 0)
            Py_RETURN_TRUE;
        Py_RETURN_FALSE;
    case Py_LT:
        if (ul_result < 0)
            Py_RETURN_TRUE;
        Py_RETURN_FALSE;
    case Py_EQ:
        if (ul_result == 0 and lr_result == 0)
            Py_RETURN_TRUE;
        Py_RETURN_FALSE;
    case Py_NE:
        if (ul_result != 0 or lr_result != 0)
            Py_RETURN_TRUE;
        Py_RETURN_FALSE;
    }
    assert(false);
    Py_RETURN_FALSE;
}

//static PyObject *
//TileExtent_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
//{
    //TileExtent *self;

    //self = (TileExtent*)type->tp_alloc(type, 0);
    //if (self != NULL) {
        //self->ndim = 0;
        //self->size = 0;
    //}

    //return (PyObject *) self;
//}

static int
TileExtent_init(PyObject *o, PyObject *args, PyObject *kwds)
{
    TileExtent *self = (TileExtent*)o;

    self->ndim = 0;
    self->size = 0;

    return 0;
}

static PyObject*
TileExtent_reduce(PyObject* o)
{
    TileExtent *self = (TileExtent*) o;
    PyObject *ul, *lr, *array_shape;

    ul = TileExtent_getul(self, NULL);
    lr = TileExtent_getlr(self, NULL);
    array_shape = TileExtent_getarray_shape(self, NULL);

    if (ul == NULL or lr == NULL or array_shape == NULL) {
        if (ul != NULL) Py_DECREF(ul);
        if (lr != NULL) Py_DECREF(lr);
        if (array_shape != NULL) Py_DECREF(array_shape);
        return NULL;
    }

    return Py_BuildValue("O(OOO)", create_obj, ul, lr, array_shape);
}

static PyObject *
TileExtent_to_slice(PyObject* o)
{
    TileExtent *self = (TileExtent*) o;
    PyObject *result;

    result = PyTuple_New(self->c_ex->ndim);
    RETURN_IF_NULL(result);

    for (int i = 0; i < self->c_ex->ndim; i++) {
        PyObject *slc;
        slc = PySlice_New(PyLong_FromLongLong(self->c_ex->ul[i]),
                          PyLong_FromLongLong(self->c_ex->lr[i]),
                          NULL);
        RETURN_IF_NULL(slc);
        PyTuple_SET_ITEM(result, i, slc);
    }

    return result;
}

static PyObject *
TileExtent_ravelled_pos(PyObject* o)
{
    TileExtent *self = (TileExtent*) o;
    npy_intp rpos;

    rpos = ravelled_pos(self->c_ex->ul, self->c_ex->array_shape, self->c_ex->ndim);
    return PyInt_FromLong(rpos);
}

static PyObject *
TileExtent_to_global(PyObject *o, PyObject *args)
{
    TileExtent *self = (TileExtent*) o;
    PyObject *idx;

    if (!PyArg_ParseTuple(args, "O", &idx))
        return NULL;

    return PyInt_FromLong(self->c_ex->to_global(PyInt_AsLong(idx)));
}

static long
TileExtent_hash(PyObject *o)
{
    TileExtent *self = (TileExtent*) o;
    long ret = self->c_ex->to_global(0);
    return ret;
}

static PyObject*
TileExtent_add_dim(PyObject* o)
{
    TileExtent *self = (TileExtent*) o;

    return _TileExtent_create_helper(self->c_ex->add_dim(), false);
}

static PyObject *
TileExtent_clone(PyObject* o)
{
    TileExtent *self = (TileExtent*) o;

    return _TileExtent_create_helper(self->c_ex->clone(), false);
}

static PyObject *
TileExtent_to_tuple(PyObject *o)
{
    TileExtent *self = (TileExtent*) o;
    PyObject *tuple, *ul, *lr, *array;

    tuple = PyTuple_New(3);
    ul = PyTuple_New(self->ndim);
    lr = PyTuple_New(self->ndim);
    array = PyTuple_New(self->ndim);
    if (tuple == NULL or ul == NULL or lr == NULL or array == NULL) {
        if (tuple != NULL) Py_DECREF(tuple);
        if (ul != NULL) Py_DECREF(ul);
        if (lr != NULL) Py_DECREF(lr);
        if (array != NULL) Py_DECREF(array);
    }

    for (int i = 0; i < self->ndim; i++) {
        PyTuple_SET_ITEM(ul, i, PyInt_FromLong(self->c_ex->ul[i]));
        PyTuple_SET_ITEM(lr, i, PyInt_FromLong(self->c_ex->lr[i]));
        PyTuple_SET_ITEM(array, i, PyInt_FromLong(self->c_ex->array_shape[i]));
    }
    PyTuple_SET_ITEM(tuple, 0, ul);
    PyTuple_SET_ITEM(tuple, 1, lr);
    PyTuple_SET_ITEM(tuple, 2, array);

    return tuple;
}


static PyMemberDef TileExtent_members[] = {
    {(char*)"ndim", T_INT, offsetof(TileExtent, ndim), 0, (char*)"number of dimenesions"},
    {(char*)"size", T_LONGLONG, offsetof(TileExtent, size), 0, (char*)"size"},
    {NULL} /* Sentinel */
};

static PyGetSetDef TileExtent_getseters[] = {
    {(char*)"ul", (getter)TileExtent_getul, (setter)TileExtent_setul, (char*)"ul", NULL},
    {(char*)"lr", (getter)TileExtent_getlr, (setter)TileExtent_setlr, (char*)"lr", NULL},
    {(char*)"shape", (getter)TileExtent_getshape,
     (setter)TileExtent_setshape, (char*) "shape", NULL},
    {(char*)"array_shape", (getter)TileExtent_getarray_shape,
     (setter)TileExtent_setarray_shape, (char*)"array_shape", NULL},
    {NULL} /* Sentinel */
};

static PyMethodDef TileExtent_methods[] = {
    {"__reduce__", (PyCFunction)TileExtent_reduce, METH_NOARGS,
     "__reduce__"},
    {"to_slice", (PyCFunction)TileExtent_to_slice, METH_NOARGS,
     "Respresent the extent by slices."},
    {"ravelled_pos", (PyCFunction)TileExtent_ravelled_pos, METH_NOARGS,
     "Ravel the extent."},
    {"to_global", (PyCFunction)TileExtent_to_global, METH_VARARGS,
     "Transform extent[idx] to the global index."},
    {"add_dim", (PyCFunction)TileExtent_add_dim, METH_NOARGS,
     "Add a dimension to this extent."},
    {(char*)"clone", (PyCFunction)TileExtent_clone, METH_NOARGS,
     (char*)"Clone the extent."},
    {"to_tuple", (PyCFunction)TileExtent_to_tuple, METH_VARARGS,
     "Transform the extent to a tuple"},
    {NULL} /* Sentinel */
};

static PyTypeObject TileExtentType = {
    PyObject_HEAD_INIT(NULL)
    0,                           /* ob_size */
    "extent.TileExtent",         /* tp_name */
    sizeof(TileExtent),          /* tp_basicsize */
    0,                           /* tp_itemsize */
    TileExtent_dealloc,          /* tp_dealloc */
    0,                           /* tp_print */
    0,                           /* tp_getattr */
    0,                           /* tp_setattr */
    0,                           /* tp_compare */
    TileExtent_repr,             /* tp_repr */
    0,                           /* tp_as_number */
    0,                           /* tp_as_sequence */
    0,                           /* tp_as_mapping */
    TileExtent_hash,             /* tp_hash */
    0,                           /* tp_call */
    0,                           /* tp_str */
    0,                           /* tp_getattro */
    0,                           /* tp_setattro */
    0,                           /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,          /* tp_flags */
    "extentobjects",             /* tp_doc */
    0,                           /* tp_traverse */
    0,                           /* tp_clear */
    TileExtent_richcompare,      /* tp_richcompare */
    0,                           /* tp_weaklistoffset */
    0,                           /* tp_iter */
    0,                           /* tp_iternext */
    TileExtent_methods,          /* tp_methods */
    TileExtent_members,          /* tp_members */
    TileExtent_getseters,        /* tp_getset */
    0,                           /* tp_base */
    0,                           /* tp_dict */
    0,                           /* tp_descr_get */
    0,                           /* tp_descr_set */
    0,                           /* tp_dictoffset */
    TileExtent_init,             /* tp_init */
    0,                           /* tp_alloc */
    0,                           /* tp_new */
};

static PyObject*
_TileExtent_create_helper(CExtent *c_ex, bool return_none_if_null)
{
    TileExtent *ex;

    if (c_ex == NULL) {
        if (return_none_if_null) {
            Py_RETURN_NONE;
        } else {
            return NULL;
        }
    } else {
        ex = PyObject_New(TileExtent, &TileExtentType);
        if (ex == NULL) {
            delete c_ex;
            return NULL;
        }

        ex->size = c_ex->size;
        ex->ndim = c_ex->ndim;
        ex->c_ex = c_ex;
        return (PyObject*)ex;
    }
}



static PyObject*
create(PyObject *self, PyObject *args)
{
    npy_intp c_ul[NPY_MAXDIMS], c_lr[NPY_MAXDIMS], c_array_shape[NPY_MAXDIMS];
    int i, ndim;
    PyObject *ul, *lr, *array_shape;
    CExtent *c_ex;

    if (!PyArg_ParseTuple(args, "OOO", &ul, &lr, &array_shape))
        return NULL;

    if (!(PySequence_Check(ul) && PySequence_Check(lr) && PySequence_Check(array_shape)))
        return NULL;

    ndim = PySequence_Fast_GET_SIZE(ul);
    for (i = 0; i < ndim; i++) {
        c_ul[i] = PyInt_AsLong(PySequence_Fast_GET_ITEM(ul, i));
        c_lr[i] = PyInt_AsLong(PySequence_Fast_GET_ITEM(lr, i));
    }

    if (array_shape != Py_None) {
        for (i = 0; i < ndim; i++) {
            c_array_shape[i] = PyInt_AsLong(PySequence_Fast_GET_ITEM(array_shape, i));
        }
        c_ex = extent_create(c_ul, c_lr, c_array_shape, ndim);
    } else {
        c_ex = extent_create(c_ul, c_lr, NULL, ndim);
    }
    return _TileExtent_create_helper(c_ex, true);
}

static PyObject*
from_shape(PyObject *self, PyObject *args)
{
    PyObject *list;

    if (!PyArg_ParseTuple(args, "O", &list))
        return NULL;

    npy_intp shape[NPY_MAXDIMS];
    int ndim = PySequence_Size(list);
    for (int i = 0; i < ndim; i++) {
        shape[i] = PyInt_AsLong(PySequence_GetItem(list, i));
    }

    return _TileExtent_create_helper(extent_from_shape(shape, ndim), false);
}

static PyObject*
from_tuple(PyObject *self, PyObject *args)
{
    PyObject *tuple;

    if (!PyArg_ParseTuple(args, "O", &tuple))
        return NULL;

    return create(self, tuple);
}

static PyObject*
intersection(PyObject *self, PyObject *args)
{
    TileExtent *ex_a, *ex_b;

    if (!PyArg_ParseTuple(args, "OO", (PyObject **)&ex_a, (PyObject **)&ex_b))
        return NULL;

    return _TileExtent_create_helper(intersection(ex_a->c_ex, ex_b->c_ex), true);
}

static PyObject*
compute_slice(PyObject *self, PyObject *args)
{
    PyObject *slice;
    TileExtent *ex;

    if (!PyArg_ParseTuple(args, "OO", (PyObject **)&ex, &slice))
        return NULL;

    CSliceIdx cslice_idx(slice, ex->ndim, ex->c_ex->shape);
    return _TileExtent_create_helper(compute_slice(ex->c_ex, cslice_idx), true);
}

static PyObject*
offset_from(PyObject *self, PyObject *args)
{
    TileExtent *ex_a, *ex_b;

    if (!PyArg_ParseTuple(args, "OO", (PyObject **)&ex_a, (PyObject **)&ex_b))
        return NULL;

    return _TileExtent_create_helper(offset_from(ex_a->c_ex, ex_b->c_ex), true);
}

static PyObject*
offset_slice(PyObject *self, PyObject *args)
{
    TileExtent *ex_a, *ex_b;
    CSlice slice[NPY_MAXDIMS];

    if (!PyArg_ParseTuple(args, "OO", (PyObject **)&ex_a, (PyObject **)&ex_b))
        return NULL;

    offset_slice(ex_a->c_ex, ex_b->c_ex, slice);
    PyObject *tuple = PyTuple_New(ex_a->ndim);
    RETURN_IF_NULL(tuple);
    for (int i = 0; i < ex_a->ndim; i++) {
        PyObject *slc;
        slc = PySlice_New(PyLong_FromLongLong(slice[i].start),
                          PyLong_FromLongLong(slice[i].stop),
                          NULL);
        RETURN_IF_NULL(slc);
        PyTuple_SET_ITEM(tuple, i, slc);
    }
    return tuple;
}

static PyObject*
from_slice(PyObject *self, PyObject *args)
{
    PyObject *slice, *shape_obj;
    npy_intp shape[NPY_MAXDIMS];
    int i, ndim;

    if (!PyArg_ParseTuple(args, "OO", (PyObject **)&slice, &shape_obj))
        return NULL;

    ndim = PySequence_Size(shape_obj);
    for (i = 0; i < ndim; i++) {
        shape[i] = PyInt_AsLong(PySequence_GetItem(shape_obj, i));
    }
    CSliceIdx cslice_idx(slice, ndim, shape);
    return _TileExtent_create_helper(from_slice(cslice_idx, shape, ndim), false);
}

static PyObject*
drop_axis(PyObject *self, PyObject *args)
{
    TileExtent *ex;
    PyObject *axis;

    if (!PyArg_ParseTuple(args, "OO", (PyObject **)&ex, &axis))
        return NULL;

    if (axis == Py_None) {
       return _TileExtent_create_helper(extent_create(NULL, NULL, NULL, 0), false);
    } else {
       return _TileExtent_create_helper(drop_axis(ex->c_ex, PyInt_AsLong(axis)), false);
    }
}

static PyObject*
index_for_reduction(PyObject *self, PyObject *args)
{
    return drop_axis(self, args);
}

/*
static PyObject*
shape_for_reduction(PyObject *self, PyObject *args)
{
    PyObject *shape, *axis;
    PyObject *list;

    if (!PyArg_ParseTuple(args, "OO", &shape, &axis))
        return NULL;

    if (axis == Py_None) {
        return PyTuple_New(0);
    } else {
        list = PySequence_List(shape);
        RETURN_IF_NULL(list);
        if (PySequence_DelItem(list, PyInt_AsLong(axis))) {
            return NULL;
        }
        return list;
    }
}
*/

static PyObject*
find_shape(PyObject *self, PyObject *args)
{
    PyObject *list;
    TileExtent *ex = NULL;

    if (!PyArg_ParseTuple(args, "O", &list))
        return NULL;

    int i, num_of_ex = PySequence_Size(list);
    CExtent **extents = new CExtent*[num_of_ex];
    npy_intp shape[NPY_MAXDIMS];
    for (i = 0; i < num_of_ex; i++) {
        ex = (TileExtent*) PySequence_GetItem(list, i);
        extents[i] = ex->c_ex;
    }
    find_shape(extents, num_of_ex, shape);
    list = PyTuple_New(ex->ndim);
    RETURN_IF_NULL(list);
    for (i = 0; i < ex->ndim; i++) {
        PyTuple_SET_ITEM(list, i, PyLong_FromLongLong(shape[i]));
    }
    return list;
}

/*
static PyObject*
shapes_match(PyObject *self, PyObject *args)
{
    TileExtent *ex_a, *ex_b;

    if (!PyArg_ParseTuple(args, "OO", (PyObject **)&ex_a, (PyObject **)&ex_b))
        return NULL;

    if (shapes_match(ex_a->c_ex, ex_b->c_ex)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
*/

static PyObject*
unravelled_pos(PyObject *self, PyObject *args)
{
    PyObject *idx_obj, *array_shape, *result;

    if (!PyArg_ParseTuple(args, "OO", &idx_obj, &array_shape))
        return NULL;

    npy_intp idx = PyInt_AsLong(idx_obj);
    int ndim = PyTuple_Size(array_shape);

    result = PyTuple_New(ndim);
    RETURN_IF_NULL(result);
    for (int i = ndim - 1; i >= 0; i--) {
        npy_intp item = PyInt_AsLong(PySequence_GetItem(array_shape, i));
        PyTuple_SET_ITEM(result, i, PyLong_FromLongLong(idx % item));
        idx /= item;
    }

    return result;
}

static PyObject*
ravelled_pos(PyObject *self, PyObject *args)
{
    PyObject *idx, *array_shape;

    if (!PyArg_ParseTuple(args, "OO", &idx, &array_shape))
        return NULL;

    npy_intp rpos = 0, mul = 1;
    int ndim = PySequence_Size(array_shape);

    for (int i = ndim - 1; i >= 0; i--) {
        rpos += mul * PyInt_AsLong(PySequence_GetItem(idx, i));
        mul *= PyInt_AsLong(PySequence_GetItem(array_shape, i));
    }

    return PyInt_FromLong(rpos);
}

static PyMethodDef extent_methods[] = {
    {"create", create, METH_VARARGS, ""},
    {"from_shape", from_shape, METH_VARARGS, ""},
    {"from_tuple", from_tuple, METH_VARARGS, ""},
    {"intersection", intersection, METH_VARARGS, ""},
    {"compute_slice", compute_slice, METH_VARARGS, ""},
    {"offset_from", offset_from, METH_VARARGS, ""},
    {"offset_slice", offset_slice, METH_VARARGS, ""},
    {"from_slice", from_slice, METH_VARARGS, ""},
    {"drop_axis", drop_axis, METH_VARARGS, ""},
    {"index_for_reduction", index_for_reduction, METH_VARARGS, ""},
    {"find_shape", find_shape, METH_VARARGS, ""},
    {"unravelled_pos", unravelled_pos, METH_VARARGS, ""},
    {"ravelled_pos", ravelled_pos, METH_VARARGS, ""},

    /**
     * These methods are noly used from Python and do not create
     * any extent. Therefore, they don't have to be implemented
     * by the native Python API
     */

    //{"shapes_match", shapes_match, METH_VARARGS, ""},
    //{"shape_for_reduction", shape_for_reduction, METH_VARARGS, ""},
    //{"find_overlapping", find_overlapping, METH_VARARGS, ""},
    //{"all_nonzero_shape", all_nonzero_shape, METH_VARARGS, ""},
    //{"find_rect"}, find_rect, METH_VARARGS, ""},
    //{"is_complete", is_complete, METH_VARARGS, ""},
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
init_cextent_py_if(void)
{
    //PyObject* m;

    TileExtentType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&TileExtentType) < 0)
        return;

    mod = Py_InitModule3("_cextent_py_if", extent_methods,
                         "Python interface for cextent module");

    Py_INCREF(&TileExtentType);
    PyModule_AddObject(mod, "TileExtent", (PyObject *)&TileExtentType);
    create_obj = PyObject_GetAttrString(mod, "create");
}

