#include <Python.h>

#include "umath.h"

#include "dtype.h"
#include "static_string.h"
#include "string.h"

static NPY_CASTING
multiply_resolve_descriptors(
        struct PyArrayMethodObject_tag *NPY_UNUSED(method),
        PyArray_DTypeMeta *dtypes[], PyArray_Descr *given_descrs[],
        PyArray_Descr *loop_descrs[], npy_intp *NPY_UNUSED(view_offset))
{
    PyArray_Descr *ldescr = given_descrs[0];
    PyArray_Descr *rdescr = given_descrs[1];
    Py_INCREF(ldescr);
    loop_descrs[0] = ldescr;
    Py_INCREF(rdescr);
    loop_descrs[1] = rdescr;

    StringDTypeObject *odescr = NULL;

    if (dtypes[0] == (PyArray_DTypeMeta *)&StringDType) {
        odescr = (StringDTypeObject *)ldescr;
    }
    else {
        odescr = (StringDTypeObject *)rdescr;
    }

    loop_descrs[2] = (PyArray_Descr *)new_stringdtype_instance(
            odescr->na_object, odescr->coerce);

    return NPY_NO_CASTING;
}

#define MULTIPLY_IMPL(shortname)                                            \
    static int multiply_loop_core_##shortname(                              \
            npy_intp N, char *sin, char *iin, char *out, npy_intp s_stride, \
            npy_intp i_stride, npy_intp o_stride)                           \
    {                                                                       \
        ss *is = NULL, *os = NULL;                                          \
                                                                            \
        while (N--) {                                                       \
            is = (ss *)sin;                                                 \
            npy_##shortname factor = *(npy_##shortname *)iin;               \
            os = (ss *)out;                                                 \
            size_t newlen = (size_t)((is->len) * factor);                   \
                                                                            \
            ssfree(os);                                                     \
            if (ssnewemptylen(newlen, os) < 0) {                            \
                return -1;                                                  \
            }                                                               \
                                                                            \
            for (size_t i = 0; i < (size_t)factor; i++) {                   \
                memcpy(os->buf + i * is->len, is->buf, is->len);            \
            }                                                               \
            os->buf[newlen] = '\0';                                         \
                                                                            \
            sin += s_stride;                                                \
            iin += i_stride;                                                \
            out += o_stride;                                                \
        }                                                                   \
        return 0;                                                           \
    }                                                                       \
                                                                            \
    static int multiply_right_##shortname##_strided_loop(                   \
            PyArrayMethod_Context *NPY_UNUSED(context), char *const data[], \
            npy_intp const dimensions[], npy_intp const strides[],          \
            NpyAuxData *NPY_UNUSED(auxdata))                                \
    {                                                                       \
        npy_intp N = dimensions[0];                                         \
        char *in1 = data[0];                                                \
        char *in2 = data[1];                                                \
        char *out = data[2];                                                \
        npy_intp in1_stride = strides[0];                                   \
        npy_intp in2_stride = strides[1];                                   \
        npy_intp out_stride = strides[2];                                   \
                                                                            \
        return multiply_loop_core_##shortname(N, in1, in2, out, in1_stride, \
                                              in2_stride, out_stride);      \
    }                                                                       \
                                                                            \
    static int multiply_left_##shortname##_strided_loop(                    \
            PyArrayMethod_Context *NPY_UNUSED(context), char *const data[], \
            npy_intp const dimensions[], npy_intp const strides[],          \
            NpyAuxData *NPY_UNUSED(auxdata))                                \
    {                                                                       \
        npy_intp N = dimensions[0];                                         \
        char *in1 = data[0];                                                \
        char *in2 = data[1];                                                \
        char *out = data[2];                                                \
        npy_intp in1_stride = strides[0];                                   \
        npy_intp in2_stride = strides[1];                                   \
        npy_intp out_stride = strides[2];                                   \
                                                                            \
        return multiply_loop_core_##shortname(N, in2, in1, out, in2_stride, \
                                              in1_stride, out_stride);      \
    }

MULTIPLY_IMPL(int8);
MULTIPLY_IMPL(int16);
MULTIPLY_IMPL(int32);
MULTIPLY_IMPL(int64);
MULTIPLY_IMPL(uint8);
MULTIPLY_IMPL(uint16);
MULTIPLY_IMPL(uint32);
MULTIPLY_IMPL(uint64);
#if NPY_SIZEOF_BYTE == NPY_SIZEOF_SHORT
MULTIPLY_IMPL(byte);
MULTIPLY_IMPL(ubyte);
#endif
#if NPY_SIZEOF_SHORT == NPY_SIZEOF_INT
MULTIPLY_IMPL(short);
MULTIPLY_IMPL(ushort);
#endif
#if NPY_SIZEOF_INT == NPY_SIZEOF_LONG
MULTIPLY_IMPL(long);
MULTIPLY_IMPL(ulong);
#endif
#if NPY_SIZEOF_LONGLONG == NPY_SIZEOF_LONG
MULTIPLY_IMPL(longlong);
MULTIPLY_IMPL(ulonglong);
#endif

static NPY_CASTING
binary_resolve_descriptors(struct PyArrayMethodObject_tag *NPY_UNUSED(method),
                           PyArray_DTypeMeta *NPY_UNUSED(dtypes[]),
                           PyArray_Descr *given_descrs[],
                           PyArray_Descr *loop_descrs[],
                           npy_intp *NPY_UNUSED(view_offset))
{
    PyObject *na_obj1 = ((StringDTypeObject *)given_descrs[0])->na_object;
    PyObject *na_obj2 = ((StringDTypeObject *)given_descrs[1])->na_object;

    // RichCompareBool has a short-circuit pointer comparison fast path.
    int eq_res = PyObject_RichCompareBool(na_obj1, na_obj2, Py_EQ);

    if (eq_res < 0) {
        return (NPY_CASTING)-1;
    }

    if (eq_res != 1) {
        PyErr_SetString(PyExc_TypeError,
                        "Can only do binary operations with StringDType "
                        "instances that share an na_object.");
        return (NPY_CASTING)-1;
    }

    Py_INCREF(given_descrs[0]);
    loop_descrs[0] = given_descrs[0];
    Py_INCREF(given_descrs[1]);
    loop_descrs[1] = given_descrs[1];
    Py_INCREF(given_descrs[1]);
    loop_descrs[2] = given_descrs[1];

    return NPY_NO_CASTING;
}

static int
add_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                 char *const data[], npy_intp const dimensions[],
                 npy_intp const strides[], NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    char *out = data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    ss *s1 = NULL, *s2 = NULL, *os = NULL;

    while (N--) {
        int newlen = 0;
        s1 = (ss *)in1;
        s2 = (ss *)in2;
        os = (ss *)out;
        newlen = s1->len + s2->len;

        ssfree(os);
        if (ssnewemptylen(newlen, os) < 0) {
            return -1;
        }

        memcpy(os->buf, s1->buf, s1->len);
        memcpy(os->buf + s1->len, s2->buf, s2->len);
        os->buf[newlen] = '\0';

        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }
    return 0;
}

static int
maximum_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                     char *const data[], npy_intp const dimensions[],
                     npy_intp const strides[], NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    char *out = data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    while (N--) {
        if (compare(in1, in2, NULL) > 0) {
            // Only copy *out* to *in1* if they point to different locations;
            // for *arr.max()* they point to the same address.
            if (in1 != out) {
                ssfree((ss *)out);
                if (ssdup((ss *)in1, (ss *)out) < 0) {
                    return -1;
                }
            }
        }
        else {
            ssfree((ss *)out);
            if (ssdup((ss *)in2, (ss *)out) < 0) {
                return -1;
            }
        }
        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static int
minimum_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                     char *const data[], npy_intp const dimensions[],
                     npy_intp const strides[], NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    char *out = data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    while (N--) {
        if (compare(in1, in2, NULL) < 0) {
            if (in1 != out) {
                ssfree((ss *)out);
                if (ssdup((ss *)in1, (ss *)out) < 0) {
                    return -1;
                }
            }
        }
        else {
            ssfree((ss *)out);
            if (ssdup((ss *)in2, (ss *)out) < 0) {
                return -1;
            }
        }
        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static int
string_equal_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                          char *const data[], npy_intp const dimensions[],
                          npy_intp const strides[],
                          NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    npy_bool *out = (npy_bool *)data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    ss *s1 = NULL, *s2 = NULL;

    while (N--) {
        s1 = (ss *)in1;
        s2 = (ss *)in2;
        if (ss_isnull(s1) || ss_isnull(s2)) {
            // s1 or s2 is NA
            *out = (npy_bool)0;
        }
        else if (s1->len == s2->len &&
                 strncmp(s1->buf, s2->buf, s1->len) == 0) {
            *out = (npy_bool)1;
        }
        else {
            *out = (npy_bool)0;
        }

        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static int
string_not_equal_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                              char *const data[], npy_intp const dimensions[],
                              npy_intp const strides[],
                              NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    npy_bool *out = (npy_bool *)data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    ss *s1 = NULL, *s2 = NULL;

    while (N--) {
        s1 = (ss *)in1;
        s2 = (ss *)in2;
        if (ss_isnull(s1) || ss_isnull(s2)) {
            // s1 or s2 is NA
            *out = (npy_bool)0;
        }
        else if (s1->len == s2->len &&
                 strncmp(s1->buf, s2->buf, s1->len) == 0) {
            *out = (npy_bool)0;
        }
        else {
            *out = (npy_bool)1;
        }

        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static int
string_greater_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                            char *const data[], npy_intp const dimensions[],
                            npy_intp const strides[],
                            NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    npy_bool *out = (npy_bool *)data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    ss *s1 = NULL, *s2 = NULL;

    while (N--) {
        s1 = (ss *)in1;
        s2 = (ss *)in2;
        if (ss_isnull(s1) || ss_isnull(s2)) {
            // s1 or s2 is NA
            *out = (npy_bool)0;
        }
        else if (strcmp(s1->buf, s2->buf) > 0) {
            *out = (npy_bool)1;
        }
        else {
            *out = (npy_bool)0;
        }

        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static int
string_greater_equal_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                                  char *const data[],
                                  npy_intp const dimensions[],
                                  npy_intp const strides[],
                                  NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    npy_bool *out = (npy_bool *)data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    ss *s1 = NULL, *s2 = NULL;

    while (N--) {
        s1 = (ss *)in1;
        s2 = (ss *)in2;
        if (ss_isnull(s1) || ss_isnull(s2)) {
            // s1 or s2 is NA
            *out = (npy_bool)0;
        }
        else if (strcmp(s1->buf, s2->buf) >= 0) {
            *out = (npy_bool)1;
        }
        else {
            *out = (npy_bool)0;
        }

        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static int
string_less_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                         char *const data[], npy_intp const dimensions[],
                         npy_intp const strides[],
                         NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    npy_bool *out = (npy_bool *)data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    ss *s1 = NULL, *s2 = NULL;

    while (N--) {
        s1 = (ss *)in1;
        s2 = (ss *)in2;
        if (ss_isnull(s1) || ss_isnull(s2)) {
            // s1 or s2 is NA
            *out = (npy_bool)0;
        }
        else if (strcmp(s1->buf, s2->buf) < 0) {
            *out = (npy_bool)1;
        }
        else {
            *out = (npy_bool)0;
        }

        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static int
string_less_equal_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                               char *const data[], npy_intp const dimensions[],
                               npy_intp const strides[],
                               NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in1 = data[0];
    char *in2 = data[1];
    npy_bool *out = (npy_bool *)data[2];
    npy_intp in1_stride = strides[0];
    npy_intp in2_stride = strides[1];
    npy_intp out_stride = strides[2];

    ss *s1 = NULL, *s2 = NULL;

    while (N--) {
        s1 = (ss *)in1;
        s2 = (ss *)in2;
        if (ss_isnull(s1) || ss_isnull(s2)) {
            // s1 or s2 is NA
            *out = (npy_bool)0;
        }
        else if (strcmp(s1->buf, s2->buf) <= 0) {
            *out = (npy_bool)1;
        }
        else {
            *out = (npy_bool)0;
        }

        in1 += in1_stride;
        in2 += in2_stride;
        out += out_stride;
    }

    return 0;
}

static NPY_CASTING
string_comparison_resolve_descriptors(
        struct PyArrayMethodObject_tag *NPY_UNUSED(method),
        PyArray_DTypeMeta *NPY_UNUSED(dtypes[]), PyArray_Descr *given_descrs[],
        PyArray_Descr *loop_descrs[], npy_intp *NPY_UNUSED(view_offset))
{
    Py_INCREF(given_descrs[0]);
    loop_descrs[0] = given_descrs[0];
    Py_INCREF(given_descrs[1]);
    loop_descrs[1] = given_descrs[1];

    loop_descrs[2] = PyArray_DescrFromType(NPY_BOOL);  // cannot fail

    return NPY_NO_CASTING;
}

static int
string_isnan_strided_loop(PyArrayMethod_Context *NPY_UNUSED(context),
                          char *const data[], npy_intp const dimensions[],
                          npy_intp const strides[],
                          NpyAuxData *NPY_UNUSED(auxdata))
{
    npy_intp N = dimensions[0];
    char *in = data[0];
    npy_bool *out = (npy_bool *)data[1];
    npy_intp in_stride = strides[0];
    npy_intp out_stride = strides[1];

    ss *s = NULL;

    while (N--) {
        s = (ss *)in;
        if (ss_isnull(s)) {
            *out = (npy_bool)1;
        }
        else {
            *out = (npy_bool)0;
        }

        in += in_stride;
        out += out_stride;
    }

    return 0;
}

static NPY_CASTING
string_isnan_resolve_descriptors(
        struct PyArrayMethodObject_tag *NPY_UNUSED(method),
        PyArray_DTypeMeta *NPY_UNUSED(dtypes[]), PyArray_Descr *given_descrs[],
        PyArray_Descr *loop_descrs[], npy_intp *NPY_UNUSED(view_offset))
{
    Py_INCREF(given_descrs[0]);
    loop_descrs[0] = given_descrs[0];
    loop_descrs[1] = PyArray_DescrFromType(NPY_BOOL);  // cannot fail

    return NPY_NO_CASTING;
}

/*
 * Copied from NumPy, because NumPy doesn't always use it :)
 */
static int
ufunc_promoter_internal(PyUFuncObject *ufunc, PyArray_DTypeMeta *op_dtypes[],
                        PyArray_DTypeMeta *signature[],
                        PyArray_DTypeMeta *new_op_dtypes[],
                        PyArray_DTypeMeta *final_dtype)
{
    /* If nin < 2 promotion is a no-op, so it should not be registered */
    assert(ufunc->nin > 1);
    if (op_dtypes[0] == NULL) {
        assert(ufunc->nin == 2 && ufunc->nout == 1); /* must be reduction */
        Py_INCREF(op_dtypes[1]);
        new_op_dtypes[0] = op_dtypes[1];
        Py_INCREF(op_dtypes[1]);
        new_op_dtypes[1] = op_dtypes[1];
        Py_INCREF(op_dtypes[1]);
        new_op_dtypes[2] = op_dtypes[1];
        return 0;
    }
    PyArray_DTypeMeta *common = NULL;
    /*
     * If a signature is used and homogeneous in its outputs use that
     * (Could/should likely be rather applied to inputs also, although outs
     * only could have some advantage and input dtypes are rarely enforced.)
     */
    for (int i = ufunc->nin; i < ufunc->nargs; i++) {
        if (signature[i] != NULL) {
            if (common == NULL) {
                Py_INCREF(signature[i]);
                common = signature[i];
            }
            else if (common != signature[i]) {
                Py_CLEAR(common); /* Not homogeneous, unset common */
                break;
            }
        }
    }
    Py_XDECREF(common);

    /* Otherwise, set all input operands to final_dtype */
    for (int i = 0; i < ufunc->nargs; i++) {
        PyArray_DTypeMeta *tmp = final_dtype;
        if (signature[i]) {
            tmp = signature[i]; /* never replace a fixed one. */
        }
        Py_INCREF(tmp);
        new_op_dtypes[i] = tmp;
    }
    for (int i = ufunc->nin; i < ufunc->nargs; i++) {
        Py_XINCREF(op_dtypes[i]);
        new_op_dtypes[i] = op_dtypes[i];
    }

    return 0;
}

static int
string_object_promoter(PyObject *ufunc, PyArray_DTypeMeta *op_dtypes[],
                       PyArray_DTypeMeta *signature[],
                       PyArray_DTypeMeta *new_op_dtypes[])
{
    return ufunc_promoter_internal((PyUFuncObject *)ufunc, op_dtypes,
                                   signature, new_op_dtypes,
                                   (PyArray_DTypeMeta *)&PyArray_ObjectDType);
}

static int
string_unicode_promoter(PyObject *ufunc, PyArray_DTypeMeta *op_dtypes[],
                        PyArray_DTypeMeta *signature[],
                        PyArray_DTypeMeta *new_op_dtypes[])
{
    return ufunc_promoter_internal((PyUFuncObject *)ufunc, op_dtypes,
                                   signature, new_op_dtypes,
                                   (PyArray_DTypeMeta *)&StringDType);
}

// Register a ufunc.
//
// Pass NULL for resolve_func to use the default_resolve_descriptors.
int
init_ufunc(PyObject *numpy, const char *ufunc_name, PyArray_DTypeMeta **dtypes,
           resolve_descriptors_function *resolve_func,
           PyArrayMethod_StridedLoop *loop_func, const char *loop_name,
           int nin, int nout, NPY_CASTING casting, NPY_ARRAYMETHOD_FLAGS flags)
{
    PyObject *ufunc = PyObject_GetAttrString(numpy, ufunc_name);
    if (ufunc == NULL) {
        return -1;
    }

    PyArrayMethod_Spec spec = {
            .name = loop_name,
            .nin = nin,
            .nout = nout,
            .casting = casting,
            .flags = flags,
            .dtypes = dtypes,
    };

    if (resolve_func == NULL) {
        PyType_Slot slots[] = {{NPY_METH_strided_loop, loop_func}, {0, NULL}};
        spec.slots = slots;
    }
    else {
        PyType_Slot slots[] = {{NPY_METH_resolve_descriptors, resolve_func},
                               {NPY_METH_strided_loop, loop_func},
                               {0, NULL}};
        spec.slots = slots;
    }

    if (PyUFunc_AddLoopFromSpec(ufunc, &spec) < 0) {
        Py_DECREF(ufunc);
        return -1;
    }

    Py_DECREF(ufunc);
    return 0;
}

int
add_promoter(PyObject *numpy, const char *ufunc_name,
             PyArray_DTypeMeta *ldtype, PyArray_DTypeMeta *rdtype,
             PyArray_DTypeMeta *edtype, promoter_function *promoter_impl)
{
    PyObject *ufunc = PyObject_GetAttrString(numpy, ufunc_name);

    if (ufunc == NULL) {
        return -1;
    }

    PyObject *DType_tuple = PyTuple_Pack(3, ldtype, rdtype, edtype);

    if (DType_tuple == NULL) {
        Py_DECREF(ufunc);
        return -1;
    }

    PyObject *promoter_capsule = PyCapsule_New((void *)promoter_impl,
                                               "numpy._ufunc_promoter", NULL);

    if (promoter_capsule == NULL) {
        Py_DECREF(ufunc);
        Py_DECREF(DType_tuple);
        return -1;
    }

    if (PyUFunc_AddPromoter(ufunc, DType_tuple, promoter_capsule) < 0) {
        Py_DECREF(promoter_capsule);
        Py_DECREF(DType_tuple);
        Py_DECREF(ufunc);
        return -1;
    }

    Py_DECREF(promoter_capsule);
    Py_DECREF(DType_tuple);
    Py_DECREF(ufunc);

    return 0;
}

#define INIT_MULTIPLY(typename, shortname)                                 \
    PyArray_DTypeMeta *multiply_right_##shortname##_types[] = {            \
            (PyArray_DTypeMeta *)&StringDType, &PyArray_##typename##DType, \
            (PyArray_DTypeMeta *)&StringDType};                            \
                                                                           \
    if (init_ufunc(numpy, "multiply", multiply_right_##shortname##_types,  \
                   &multiply_resolve_descriptors,                          \
                   &multiply_right_##shortname##_strided_loop,             \
                   "string_multiply", 2, 1, NPY_NO_CASTING, 0) < 0) {      \
        goto error;                                                        \
    }                                                                      \
                                                                           \
    PyArray_DTypeMeta *multiply_left_##shortname##_types[] = {             \
            &PyArray_##typename##DType, (PyArray_DTypeMeta *)&StringDType, \
            (PyArray_DTypeMeta *)&StringDType};                            \
                                                                           \
    if (init_ufunc(numpy, "multiply", multiply_left_##shortname##_types,   \
                   &multiply_resolve_descriptors,                          \
                   &multiply_left_##shortname##_strided_loop,              \
                   "string_multiply", 2, 1, NPY_NO_CASTING, 0) < 0) {      \
        goto error;                                                        \
    }

int
init_ufuncs(void)
{
    PyObject *numpy = PyImport_ImportModule("numpy");
    if (numpy == NULL) {
        return -1;
    }

    static char *comparison_ufunc_names[6] = {"equal",   "not_equal",
                                              "greater", "greater_equal",
                                              "less",    "less_equal"};

    PyArray_DTypeMeta *comparison_dtypes[] = {
            (PyArray_DTypeMeta *)&StringDType,
            (PyArray_DTypeMeta *)&StringDType, &PyArray_BoolDType};

    if (init_ufunc(numpy, "equal", comparison_dtypes,
                   &string_comparison_resolve_descriptors,
                   &string_equal_strided_loop, "string_equal", 2, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    if (init_ufunc(numpy, "not_equal", comparison_dtypes,
                   &string_comparison_resolve_descriptors,
                   &string_not_equal_strided_loop, "string_not_equal", 2, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    if (init_ufunc(numpy, "greater", comparison_dtypes,
                   &string_comparison_resolve_descriptors,
                   &string_greater_strided_loop, "string_greater", 2, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    if (init_ufunc(numpy, "greater_equal", comparison_dtypes,
                   &string_comparison_resolve_descriptors,
                   &string_greater_equal_strided_loop, "string_greater_equal",
                   2, 1, NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    if (init_ufunc(numpy, "less", comparison_dtypes,
                   &string_comparison_resolve_descriptors,
                   &string_less_strided_loop, "string_less", 2, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    if (init_ufunc(numpy, "less_equal", comparison_dtypes,
                   &string_comparison_resolve_descriptors,
                   &string_less_equal_strided_loop, "string_less_equal", 2, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    for (int i = 0; i < 6; i++) {
        if (add_promoter(numpy, comparison_ufunc_names[i],
                         (PyArray_DTypeMeta *)&StringDType,
                         &PyArray_UnicodeDType, &PyArray_BoolDType,
                         string_unicode_promoter) < 0) {
            goto error;
        }

        if (add_promoter(numpy, comparison_ufunc_names[i],
                         &PyArray_UnicodeDType,
                         (PyArray_DTypeMeta *)&StringDType, &PyArray_BoolDType,
                         string_unicode_promoter) < 0) {
            goto error;
        }

        if (add_promoter(numpy, comparison_ufunc_names[i],
                         &PyArray_ObjectDType,
                         (PyArray_DTypeMeta *)&StringDType, &PyArray_BoolDType,
                         &string_object_promoter) < 0) {
            goto error;
        }

        if (add_promoter(numpy, comparison_ufunc_names[i],
                         (PyArray_DTypeMeta *)&StringDType,
                         &PyArray_ObjectDType, &PyArray_BoolDType,
                         &string_object_promoter) < 0) {
            goto error;
        }
    }

    PyArray_DTypeMeta *isnan_dtypes[] = {(PyArray_DTypeMeta *)&StringDType,
                                         &PyArray_BoolDType};

    if (init_ufunc(numpy, "isnan", isnan_dtypes,
                   &string_isnan_resolve_descriptors,
                   &string_isnan_strided_loop, "string_isnan", 1, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    PyArray_DTypeMeta *binary_dtypes[] = {
            (PyArray_DTypeMeta *)&StringDType,
            (PyArray_DTypeMeta *)&StringDType,
            (PyArray_DTypeMeta *)&StringDType,
    };

    if (init_ufunc(numpy, "maximum", binary_dtypes, binary_resolve_descriptors,
                   &maximum_strided_loop, "string_maximum", 2, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    if (init_ufunc(numpy, "minimum", binary_dtypes, binary_resolve_descriptors,
                   &minimum_strided_loop, "string_minimum", 2, 1,
                   NPY_NO_CASTING, 0) < 0) {
        goto error;
    }

    if (init_ufunc(numpy, "add", binary_dtypes, binary_resolve_descriptors,
                   &add_strided_loop, "string_add", 2, 1, NPY_NO_CASTING,
                   0) < 0) {
        goto error;
    }

    INIT_MULTIPLY(Int8, int8);
    INIT_MULTIPLY(Int16, int16);
    INIT_MULTIPLY(Int32, int32);
    INIT_MULTIPLY(Int64, int64);
    INIT_MULTIPLY(UInt8, uint8);
    INIT_MULTIPLY(UInt16, uint16);
    INIT_MULTIPLY(UInt32, uint32);
    INIT_MULTIPLY(UInt64, uint64);
#if NPY_SIZEOF_BYTE == NPY_SIZEOF_SHORT
    INIT_MULTIPLY(Byte, byte);
    INIT_MULTIPLY(UByte, ubyte);
#endif
#if NPY_SIZEOF_SHORT == NPY_SIZEOF_INT
    INIT_MULTIPLY(Short, short);
    INIT_MULTIPLY(UShort, ushort);
#endif
#if NPY_SIZEOF_INT == NPY_SIZEOF_LONG
    INIT_MULTIPLY(Long, long);
    INIT_MULTIPLY(ULong, ulong);
#endif
#if NPY_SIZEOF_LONGLONG == NPY_SIZEOF_LONG
    INIT_MULTIPLY(LongLong, longlong);
    INIT_MULTIPLY(ULongLong, ulonglong);
#endif

    Py_DECREF(numpy);
    return 0;

error:
    Py_DECREF(numpy);
    return -1;
}
