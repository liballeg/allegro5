import SCons
import re, os

# def getArgumentOption(name, default):
#     import SCons.Script.ARGUMENTS
#     arg = ARGUMENTS.get(name,default)
#     if arg == "yes" or arg == "1":
#         return 1
#     return 0

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

# Global state to handle configuration things
configure_state = SimpleHash()
install = 'install'

def read_cmake_list(name):
    """
    Read a cmake files list and return a dictionary with each cmake variable
    matched to a list of filenames.

    This makes it easy to add/remove files, as only the cmake list needs to be
    modified and scons will automatically pick up the changes.
    """
    in_set = False
    lists = {}
    for line in open(name):
        import re
        comment = re.compile( "^\s*#" )
        if comment.match(line): continue
        if line.startswith("set"):
            current = []
            name = line[4:].strip()
            lists[name] = current
            in_set = True
        elif in_set:
            w = line.strip(" \t\r\n")
            if w == ")":
                in_set = False
            elif w: current.append(w)
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
    """
    Parse a cmake file and return a hashmap that contains
    defines for the variables set in the cmake file
    """
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

def mergeEnv(env1, env2):
    """Copy environment variables from env2 to env1. Returns a new environment"""
    env = env1.Clone()
    try:
        env.Append(CCFLAGS = env2['CCFLAGS'])
    except KeyError:
        pass
    try:
        env.Append(LIBS = env2['LIBS'])
    except KeyError:
        pass
    try:
        env.Append(CPPFLAGS = env2['CPPFLAGS'])
    except KeyError:
        pass
    try:
        env.Append(LIBPATH = env2['LIBPATH'])
    except KeyError:
        pass
    try:
        env.Append(LINKFLAGS = env2['LINKFLAGS'])
    except KeyError:
        pass

    return env


def do_configure(name, context, tests, setup_platform, cmake_file, h_file, reconfigure):
    """
    Run a set of configure tests (which should be invoked by setup_platform())
    and save the result in a global variable, configure_state, for other variants
    of the same SConscript to use. A global .cfg and .h file are also generated in
    the scons_build/configure directory and act as the cache for future invocations
    of the build.
    """
    import os, ConfigParser
    noconfig = False
    config_settings = ["CCFLAGS", "CPPPATH", "CPPFLAGS", "LIBS", "LIBPATH", "LINKFLAGS"]

    platform = SimpleHash()
    env = context.defaultEnvironment()

    configured = configure_state[name]
    config_file = None

    main_dir = context.getGlobalDir() + '/configure/'

    try: os.mkdir(main_dir)
    except OSError: pass

    if configured:
        noconfig = True
    # elif not getArgumentOption("config",0):
    elif not reconfigure:
        if os.path.exists(main_dir + h_file):
            print "Re-using old %s settings" % name
            noconfig = True
    
    if noconfig:
        settings = ConfigParser.ConfigParser()
        if configured:
            platform, settings = configure_state[name]
        else:
            platform = readAutoHeader(main_dir + h_file)
            settings.read(main_dir + name + ".cfg")

        env = context.defaultEnvironment()
        for setting in config_settings:
            try:
                eval("env.Append(%s = %s)" % (
                    setting, settings.get(name, setting)))
            except ConfigParser.NoOptionError:
                pass
    else:
        config = env.Configure(custom_tests = tests)
        env = setup_platform(platform, config)

        settings = ConfigParser.ConfigParser()
        settings.add_section(name)
        for setting in config_settings:
            val = env.get(setting, None)
            if not val: continue
            if hasattr(val, "data"): val = val.data # for CCFLAGS
            if not type(val) == list: val = [val]
            settings.set(name, setting, str(val))
        settings.write(file(main_dir + name + ".cfg", "w"))
        if cmake_file:
            config_file = parse_cmake_h(env, platform, cmake_file,
                '#' + main_dir + h_file)

        configure_state[name + "_h"] = config_file

    if cmake_file:
        header = parse_cmake_h(env, platform, cmake_file, h_file)
        if configure_state[name + "_h"] != False:
            env.Depends(header, configure_state[name + "_h"])
        
    configure_state[name] = [platform, settings]

    return [platform, env]

# FIXME
# This is a small hack which makes a wrapper class around SCons options, which
# ignores double options. We need this because we call our options for each
# build variant. Doing that is wrong, we should call each option only once, but
# for now this is an easy fix.
only_once = {}
class Options:
    def __init__(self, context, path, ARGUMENTS):
        if path not in only_once:
            only_once[path] = True
            self.double = False
        else:
            self.double = True
        self.path = os.path.join(context.getGlobalDir(), path)
        self.o = SCons.Options.Options(self.path, ARGUMENTS)

    def Save(self, env):
        if self.double: return
        self.o.Save(self.path, env)
    
    def GenerateHelpText(self, env):
        if self.double: return ""
        return self.o.GenerateHelpText(env)

    # For all other methods, use the original scons one.
    def __getattr__(self, attr):
        return getattr(self.o, attr)