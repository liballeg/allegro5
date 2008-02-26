import SCons

def read_cmake_list(name):
    """
    Read a cmake files list and return a dictionary with each cmake variable
    matched to a list of filenames.

    This makes it easy to add/remove files, as only the cmake list needs to be
    modified and scons will automatically pick up the changes.
    """
    lists = {}
    for line in open(name):
        if line.startswith("set"):
            current = []
            name = line[4:].strip()
            lists[name] = current
        else:
            w = line.strip("( )\t\r\n")
            if w: current.append(w)
    return lists

def generate_alplatf_h(env, define):
    """
    Add a builder to the given environment to build alplatf.h with the given
    define.
    """
    def func(target, source, env):
        writer = open( target[0].path, 'w' )
        writer.write('#define %s\n' % define)
        writer.close()
        return 0

    # Generate alplatf.h
    platformHeader = SCons.Builder.Builder(action = func)
    # Add the builder to scons.
    env.Append(BUILDERS = { "PlatformHeader" : platformHeader })
    # Create alplatf.h based on alplatf.h.cmake
    return env.PlatformHeader('#include/allegro5/platform/alplatf.h',
        '#include/allegro5/platform/alplatf.h.cmake')
