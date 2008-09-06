
def getOption(context, name, default = 0):
    try:
        return context.getLibraryEnv()[name]
    except KeyError:
        return default

def do_build(context, source, dir, name, examples = [],
    install_headers = [], includes = [], example_libs = [], configs = [],
    libs = []):
    def build(env, libDir):
        libEnv = env.Clone()
        libEnv.Append(CPPPATH = includes)
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
        lib = context.makeLibrary( libEnv )( libDir + ('/%s' % name), source)

        exampleEnv = env.Clone()
        exampleEnv.Append(CPPPATH = includes)
        exampleEnv.Append(LIBS = [context.libraryName(name)])
        exampleEnv.Append(LIBS = example_libs + libs)
    
        def install():
            installDir = getOption(context, 'install','/usr/local')
            targets = []
            def add(t):
                targets.append(t)
            add(env.Install(installDir + '/lib/', lib))
            for header in install_headers:
	            add(env.Install(installDir + '/include/allegro5/', '%s' % (header)))
            return targets

        env.Alias('install', install())
        env.Alias('install-addons/%s' % name, install())

        all = lib
        env.Alias('addons', all)
        env.Alias('addons/%s' % name, all)
    
        return all

    build(context.getLibraryEnv(), 'lib' )
    # context.addExtra(build,depends = True)
