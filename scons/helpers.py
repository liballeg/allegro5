import SCons
import re

def read_cmake_list(name):
    """
    Read a cmake files list and return a dictionary with each cmake variable
    matched to a list of filenames.

    This makes it easy to add/remove files, as only the cmake list needs to be
    modified and scons will automatically pick up the changes.
    """
    lists = {}
    for line in open(name):
        if line.startswith("#"): continue
        if line.startswith("set"):
            current = []
            name = line[4:].strip()
            lists[name] = current
        else:
            w = line.strip("( )\t\r\n")
            if w: current.append(w)
    return lists

def generate_alplatf_h(env, defines):
    """
    Add a builder to the given environment to build alplatf.h with the given
    define.
    """
    def convert_cmake_h(target, source, env):
        writer = open( target[0].path, 'w' )
    for define in defines:
        writer.write('#define %s\n' % define)
        writer.close()
        return 0

    # Generate alplatf.h
    platformHeader = SCons.Builder.Builder(action = convert_cmake_h)
    # Add the builder to scons.
    env.Append(BUILDERS = { "PlatformHeader" : platformHeader })
    # Create alplatf.h based on alplatf.h.cmake
    return env.PlatformHeader('include/allegro5/platform/alplatf.h',
        'include/allegro5/platform/alplatf.h.cmake')

class SimpleHash:
    def __init__( self ):
        self.hash = dict()

    def __getitem__( self, key ):
        try:
            return self.hash[ key ]
        except KeyError:
            return False

    def __setitem__( self, key, value ):
        self.hash[ key ] = value

    def keys( self ):
        return self.hash.keys()

def define(*rest):
    n = SimpleHash()
    for i in rest:
        n[i] = True
    return n

def readAutoHeader(filename):
    """Read current config settings from the #define commands."""
    obj = SimpleHash()
    for line in file(filename):
        if line.startswith("#define "):
            line = line.split(None, 2)[1:]
            # In case the line is in the form #define X, set it to True.
            if len(line) == 1: line += [True]
            name, use = line
            obj[name] = use
    return obj

def parse_cmake_h(env, defines, src, dest):
    def parse_line(line):
        # Replace cmake variables of the form ${variable}.
        def substitute_variable(match):
            var = defines[match.group(1)]
            return str(var)
        line = re.sub(r"\$\{(.*?)\}", substitute_variable, line)
        m = re.compile('^#cmakedefine (.*)').match(line)
        if m:
            name = m.group(1)
            if defines[name]:
                return "#define %s\n" % name
            else:
                return "/* #undef %s */\n" % name
        else:
            return line

    def parse(target, source, env):
        reader = open(source[0].path, 'r')
        writer = open(target[0].path, 'w')
        # all = reader.read()
        for i in reader.readlines():
            writer.write(parse_line(i))
        reader.close()
        writer.close()
        return 0
    env2 = env.Clone()
    make = SCons.Builder.Builder(action = parse)
    env2.Append(BUILDERS = { "PlatformHeader" : make })
    return env2.PlatformHeader(dest,src)

