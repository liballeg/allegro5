/* -*- mode:C++; tab-width: 4 -*- */
/*
The zlib/libpng License

pyjpgalleg is copyright (c) 2006 by Grzegorz Adam Hankiewicz

This software is provided 'as-is', without any express or implied  
warranty. In no event will the author be held liable for any damages  
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,  
including commercial applications, and to alter it and redistribute it  
freely, subject to the following restrictions:


1. The origin of this software must not be misrepresented; you must not  
claim that you wrote the original software. If you use this software in 
a product, an acknowledgment in the product documentation would be  
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not 
be  misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
*/

#include <Python.h>
#include <jpgalleg.h>


/* Wrapper around jpegalleg_init() */
static PyObject *pyjpgalleg_init(PyObject *self, PyObject *args)
{
    int ret = jpgalleg_init();
    return Py_BuildValue("i", ret);
}


/* Methods exported by the C module. */
static PyMethodDef PyjpgallegMethods[] = {
    {"init",    pyjpgalleg_init, METH_VARARGS,
        "Initialise the jpgalleg library. Call after Allegro initialisation."},
    {NULL, NULL, 0, NULL} /* Sentinel. */
};


/* Define the initialisation function. */
PyMODINIT_FUNC initpyjpgalleg(void)
{
    (void) Py_InitModule("pyjpgalleg", PyjpgallegMethods);
}


/* Main entry point of the module for static link initialisation. */
int main(int argc, char *argv[])                                            
{
    /* Pass argv[0] to the Python interpreter */
    Py_SetProgramName(argv[0]);

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

    /* Add a static module */
    initpyjpgalleg();

    return 0;
}
