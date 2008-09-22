
def getOption(env, name, default = 0):
    try:
        return env[name]
    except KeyError:
        return default

def do_build(context, env, source, dir, name, examples = [],
    install_headers = [], includes = [], example_libs = [], configs = [],
    libs = []):
    def build(env, libDir):
        libEnv = env.Clone()
        libEnv.Append(CPPPATH = [includes,'.'])
        if context.isStatic():
            libEnv.Append(LIBS = libs)
        else:
            libEnv.Replace(LIBS = libs)

        # Allegro's include directory
        libEnv.Append(CPPPATH = ['../../include'])
        for i in configs:
            try:
                libEnv.ParseConfig(i)
            except OSError:
                print("Could not run '%s'. Please add a configure check so " +
                    "this error does not appear" % i)
                # Could not exec the configuration, bail out!
                return []
        # lib = context.makeLibrary( libEnv )( libDir + ('/%s' % name), appendDir(('addons/%s' % dir), source))
        lib = context.buildLibrary( libEnv, libDir + ('/%s' % context.libraryName(name)), source)

        exampleEnv = env.Clone()
        exampleEnv.Append(CPPPATH = includes)
        exampleEnv.Append(LIBS = [context.libraryName(name)])
        exampleEnv.Append(LIBS = example_libs + libs)
    
        def install():
            import os
            # installDir = getOption(env, 'install')
            prefix = context.temporaryInstallDir()
            targets = []
            def add(dir, t):
                path = os.path.join('..', '..', prefix, dir)
                # path = os.path.join(prefix, dir)
                targets.append(env.Install(path, t))
            add('lib', lib)
            for header in install_headers:
                add('include/allegro5/', '%s' % (header))
            return targets

        # env.Alias('install', install())
        # env.Alias('install-addons/%s' % name, install())

        all = lib
        context.alias('all-addons', all)
        context.alias('all-addons/%s' % name, all)
    
        return install()

    return build(env, 'lib')
    # context.addExtra(build,depends = True)
