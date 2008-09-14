import SCons
import re

# def getArgumentOption(name, default):
#     import SCons.Script.ARGUMENTS
#     arg = ARGUMENTS.get(name,default)
#     if arg == "yes" or arg == "1":
#         return 1
#     return 0

def read_cmake_list(name):
    """
    Read a cmake files list and return a dictionary with each cmake variable
    matched to a list of filenames.

    This makes it easy to add/remove files, as only the cmake list needs to be
    modified and scons will automatically pick up the changes.
    """
    lists = {}
    for line in open(name):
        import re
        comment = re.compile( "^\s*#" )
        if comment.match(line): continue
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

def mergeEnv(env1, env2):
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

# Global state to handle configuration things
configure_state = SimpleHash()

def do_configure(name, context, tests, setup_platform, cmake_file, h_file, reconfigure):
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
