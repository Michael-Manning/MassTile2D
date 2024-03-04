#include <Python.h>
#include <string>
#include <functional>
#include "../../MyEngine/typedefs.h"

// Function to be called from Python
static PyObject* hash_string(PyObject* self, PyObject* args) {
    const char* inputString;
    // Parse the Python argument to a C string
    if (!PyArg_ParseTuple(args, "s", &inputString)) {
        return nullptr; // In case of error
    }
    std::string str(inputString);
    // Use std::hash to hash the string
    std::size_t hash = std::hash<std::string>{}(str);
    spriteID tres = static_cast<spriteID>(hash);

    // Return the hash as a Python integer
    return PyLong_FromUnsignedLong(hash);
}

// Method definition object for this extension, makes "hash_string" callable from Python
static PyMethodDef HashMethods[] = {
    {"hash_string", hash_string, METH_VARARGS, "Hashes a string using std::hash."},
    {nullptr, nullptr, 0, nullptr} // Sentinel
};

// Module definition
static struct PyModuleDef hashmodule = {
    PyModuleDef_HEAD_INIT,
    "hash_extension", // name of module
    nullptr, // module documentation, may be nullptr
    -1, // size of per-interpreter state of the module, or -1 if the module keeps state in global variables
    HashMethods
};

// Initialization function for this module
PyMODINIT_FUNC PyInit_hash_extension(void) {
    return PyModule_Create(&hashmodule);
}
