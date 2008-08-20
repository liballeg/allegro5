
def getOption(context, name, default = 0):
    try:
        return context.getLibraryEnv()[name]
    except KeyError:
        return default

def do_build(context, source, dir, name, examples = [],
    install_headers = [], includes = [], example_libs = [], configs = []):
    def build(env, appendDir, buildDir, libDir ):
        libEnv = env.Clone()
        libEnv.Append(CPPPATH = includes)
        for i in configs:
            try:
                libEnv.ParseConfig(i)
            except OSError:
		print "Could not run '%s'. Please add a configure check so this error does not appear" % i
                # Could not exec the configuration, bail out!
                return []
        lib = context.makeLibrary( libEnv )( libDir + ('/%s' % name), appendDir(buildDir + ('/addons/%s' % dir),source))

        exampleEnv = env.Clone()
        exampleEnv.Append(CPPPATH = includes)
        exampleEnv.Append(LIBS = [context.libraryName(name)])
        exampleEnv.Append(LIBS = example_libs)
    
        build_examples = []
        def addExample(ex_name, files):
            example = exampleEnv.Program('addons/%s/%s' % (dir,ex_name), appendDir(buildDir + ('/addons/%s/' % dir), files))
            env.Alias('%s-%s' % (name,ex_name), example)
            build_examples.append(example)

        for pgm, src in examples:
            addExample(pgm, src)

        def install():
            installDir = getOption(context, 'install','/usr/local')
            targets = []
            def add(t):
                targets.append(t)
            add(env.Install(installDir + '/lib/', lib))
            for header in install_headers:
	            add(env.Install(installDir + '/include/allegro5/', 'addons/%s/%s' % (dir,header)))
            return targets

        env.Alias('install', install())
        env.Alias('install-addons/%s' % name, install())

        all = build_examples + lib
        env.Alias('addons', all)
        env.Alias('addons/%s' % name, all)
    
        return all

    context.addExtra(build,depends = True)
