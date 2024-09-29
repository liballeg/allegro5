#!/usr/bin/env python3

"""
This script will use the prototypes from "checkdocs.py -s" to concoct
a 1:1 LuaJIT FFI wrapper for Allegro. Thanks to generate_python_ctypes.py.
"""

import sys
import re
import optparse

class Allegro:
    def __init__(self):
        self.typesfw = []
        self.typesfuncptr = []
        self.typedecl = []
        self.functions = []
        self.constants = {}
        self.enums = []

    def parse_protos(self, filename):
        deferred = []

        # first pass: create all structs, but without fields
        for line in open(filename):
            name, proto = line.split(":", 1)
            proto = proto.lstrip()
            pwords = proto.split(' ')
            if name.endswith("()"):
                self.functions.append(proto)
                continue
            # anonymous structs have no name at all
            if name and not name.startswith("A5O_"):
                continue
            if name == "A5O_OGL_EXT_API":
                continue

            if pwords[0] in ('union', 'struct'):
                if '{' in proto:
                    self.typedecl.append(proto)
                else:
                    self.typesfw.append(proto)
            elif pwords[0] == 'typedef' and pwords[1] in ('union', 'struct'):
                if '{' in proto:
                    self.typedecl.append(proto)
                else:
                    self.typesfw.append(proto)
            elif proto.startswith("enum") or\
                proto.startswith("typedef enum"):
                if name:
                    self.enums.append('enum %s;' % name)
                deferred.append(("", proto))
            elif proto.startswith("#define"):
                if not name.startswith("_") and not name.startswith("GL_"):
                    i = eval(proto.split(None, 2)[2])
                    self.constants[name] = i
            else:
                mob = re.match("typedef (.*) " + name, proto)
                if mob:
                    self.typesfw.append(proto)
                else:
                    # Probably a function pointer
                    self.typesfuncptr.append(proto)

        # Second pass of prototypes now we have seen them all.
        for name, proto in deferred:
            bo = proto.find("{")
            if bo == -1:
                continue
            bc = proto.rfind("}")
            braces = proto[bo + 1:bc]

            # Calculate enums
            if proto.startswith("enum") or \
                proto.startswith("typedef enum"):

                fields = braces.split(",")
                i = 0
                for field in fields:
                    if "=" in field:
                        fname, val = field.split("=", 1)
                        fname = fname.strip()
                        # replace any 'X' (an integer value in C) with
                        # ord('X') to match up in Python
                        val = re.sub("('.')", "ord(\\1)", val)
                        try:
                            i = int(eval(val, globals(), self.constants))
                        except NameError:
                            i = val
                        except Exception:
                            raise ValueError(
                                "Exception while parsing '{}'".format(
                                    val))
                    else:
                        fname = field.strip()
                    if not fname:
                        continue
                    self.constants[fname] = i
                    try:
                        i += 1
                    except TypeError:
                        pass
                continue

def typeorder(decls, declared):
    """ Order the types by dependency. """
    decl, seen = set(), set()
    ordered = []
    todo = decls[:]

    seen.add('A5O_EVENT_TYPE')
    seen.add('A5O_KEY_MAX')
    seen.add('A5O_USER_EVENT_DESCRIPTOR')

    def info(t):
        brk, brk2 = t.find('{'), t.rfind('}')
        tbrk = t[brk+1:brk2] if brk >= 0 and brk2 >= 0 else None
        tw = t.split(' ')
        name = None
        if tw[0] in ('struct', 'union'):
            name = tw[1]
        elif tw[0] == 'typedef':
            if tw[1] in ('struct', 'union'):
                name = tw[2]
        return name, tbrk

    for t in declared:
        name, _ = info(t)
        if name:
            decl.add(name)

    reall = re.compile('(A5O_\w+\s*\*?)')
    retname = re.compile('(A5O_\w+)')
    c = 0
    while True:
        lb = len(todo)
        for t in todo[:]:
            name, brk = info(t)
            aldeps = reall.findall(brk)
            ok = True
            # print aldeps
            for rdep in aldeps:
                dep = retname.match(rdep).group(1)
                if dep.startswith('A5O_GL_'):
                    continue    # ignore
                if dep in decl and rdep[-1] == '*':
                    continue    # ok, seen type and ptr
                if not dep in seen:
                    ok = False
                    break
            if ok:
                ordered.append(t)
                todo.remove(t)
                seen.add(name)
        if len(todo) == 0:
            break
        elif lb == len(todo):
            c += 1
            assert c < 10, 'loop, bad'
    # ordered += todo
    return ordered

def main():
    p = optparse.OptionParser()
    p.add_option("-o", "--output", help="location of generated file")
    p.add_option("-p", "--protos", help="A file with all " +
        "prototypes to generate LuaJIT FFI wrappers for, one per line. "
        "Generate it with docs/scripts/checkdocs.py -p")
    p.add_option("-t", "--type", help="the library type to " +
        "use, e.g. debug")
    p.add_option("-v", "--version", help="the library version to " +
        "use, e.g. 5.1")
    options, args = p.parse_args()

    if not options.protos:
        p.print_help()
        return

    al = Allegro()

    al.parse_protos(options.protos)

    f = open(options.output, "w") if options.output else sys.stdout

    release = options.type
    version = options.version
    f.write(r"""-- Generated by generate_luajit_ffi.py
-- Release: %(release)s-%(version)s

local ffi = require 'ffi'

local allegro = {}

""" % locals())

    f.write('-- CONSTANTS\n')
    deferred = []
    for name, val in sorted(al.constants.items()):
        try:
            if isinstance(val, str):
                val = int(eval(val, globals(), al.constants))
            f.write('allegro.%s = %s\n' % (name, str(val)))
        except:
            deferred.append((name, val))

    reconst = re.compile('([A-Za-z_]\w+)')
    for name, val in deferred:
        val = reconst.sub(r'allegro.\1', val)
        f.write('allegro.%s = %s\n' % (name, str(val)))

    f.write(r"""
ffi.cdef[[

typedef uint64_t off_t;
typedef int64_t time_t;
typedef void* va_list;
typedef int al_fixed;
typedef unsigned int GLuint;
typedef unsigned int A5O_EVENT_TYPE;
enum { A5O_KEY_MAX = 227 };
""")

    # enums
    f.write('\n// ENUMS\n')
    reenum = re.compile('enum (\w+)')
    for e in al.enums:
        ename = reenum.match(e).group(1)
        f.write('typedef enum %s %s;\n' % (ename, ename))

    # types
    f.write('\n// TYPES\n')
    for t in al.typesfw:
        f.write(t + "\n")
    for t in al.typesfuncptr:
        f.write(t + "\n")
    seen = al.typesfw[:]
    for t in typeorder(al.typedecl, seen):
        f.write(t + "\n")

    # functions
    f.write('\n// FUNCTIONS\n')
    for fn in al.functions:
        f.write(fn + "\n")

    f.write(']]\n') # end of cdef

    f.write(r"""
return allegro
    """)

    f.close()

main()
