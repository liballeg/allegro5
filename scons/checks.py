import os

def CheckIntel(context):
    context.Message("Checking for intel chip... ")
    ret = context.TryCompile("""
        .globl _dummy_function
        _dummy_function:
        pushl %ebp
        movl %esp, %ebp
        leal 10(%ebx, %ecx, 4), %edx
        call *%edx
        addl %ebx, %eax
        popl %ebp
        ret
    """, ".s")

    context.Result(ret)
    return ret

def CheckMTune(context,machine):
    context.Message("Checking if -mtune is supported... ")
    tmp = context.env.Copy()
    context.env.Append(CCFLAGS = '-mtune=' + machine)
    ret = context.TryCompile("""
        int main(){}
        """, ".c");
    context.sconf.env = tmp
    context.Result(ret)
    return ret

def CheckAMD64(context):
    context.Message("Checking for AMD64 chip... ")
    ret = context.TryCompile("""
        .globl _dummy_function
        _dummy_function:
        pushq %rbp
        movl %esp, %ebp
        leal 10(%ebx, %ecx, 4), %edx
        callq *%rdx
        addl %ebx, %eax
        popq %rbp
        ret""", ".s")
    context.Result(ret)
    return ret

def CheckSparc(context):
    context.Message("Checking for SPARC chip... ")
    ret = context.TryCompile("""
        globl _dummy_function
        _dummy_function:
        pushq %rbp
        movl %esp, %ebp
        leal 10(%ebx, %ecx, 4), %edx
        callq *%rdx
        addl %ebx, %eax
        popq %rbp
        ret""", ".s")
    context.Result(ret)
    return ret

def CheckMMX(context):
    context.Message("Checking for MMX... ")
    ret = context.TryCompile("""
        .globl _dummy_function
        _dummy_function:
        pushl %ebp
        movl %esp, %ebp
        movq -8(%ebp), %mm0
        movd %edi, %mm1
        punpckldq %mm1, %mm1
        psrld $ 19, %mm7
        pslld $ 10, %mm6
        por %mm6, %mm7
        paddd %mm1, %mm0
        emms
        popl %ebp
        ret""", ".s")
    context.Result(ret)
    return ret

def CheckSSE(context):
    context.Message("Checking for SSE... ")
    ret = context.TryCompile("""
        .globl _dummy_function
        _dummy_function:
        pushl %ebp
        movl %esp, %ebp
        movq -8(%ebp), %mm0
        movd %edi, %mm1
        punpckldq %mm1, %mm1
        psrld $ 10, %mm3
        maskmovq %mm3, %mm1
        paddd %mm1, %mm0
        emms
        popl %ebp
        ret""", ".s")
    context.Result(ret)
    return ret

def CheckASMUnderscores(context):
    context.Message("Checking for asm prefix before symbol... ")
    ret = context.TryLink("""
        int test_for_underscore(void);
        asm (".globl _test_for_undercore\\n"
             "_test_for_underscore:");
        int main(){
            test_for_underscore();
        }
    """, ".c")
    context.Result(ret)
    return ret

def CheckProcFS(context):
    context.Message("Checking for System V sys/procfs... ")
    ret = context.TryLink("""
        #include <sys/procfs.h>
        #include <sys/ioctl.h>

        int main(){
            struct prpsinfo_t psinfo;
            ioctl(0, PIOCPSINFO, &psinfo);
        }
        """, ".c")
    context.Result(ret)
    return ret

def CheckProcFSArgCV(context):
    context.Message("Checking if sys/procfs.h tells us argc/argv... ")
    ret = context.TryLink("""
        #include <sys/procfs.h>
        int main(){
            prpsinfo_t psinfo;
            psinfo.pr_argc = 0;
        }
        """, ".c")
    context.Result(ret)
    return ret

def CheckBigEndian(context):
    context.Message("Checking for big endianness... ")
    import sys
    if sys.byteorder != 'big':
        context.Result(0)
        return 0
    context.Result(1)
    return 1

def CheckLittleEndian(context):
    context.Message("Checking for little endianness... ")
    import sys
    if sys.byteorder != 'little':
        context.Result(0)
        return 0
    context.Result(1)
    return 1

def CheckDarwin(context):
    context.Message("Checking if host is Darwin... ")
    import string
    uname = os.uname()
    if string.lower(uname[ 0 ]) == "darwin":
        context.Result(1)
        return 1
    context.Result(0)
    return 0

def CheckLinux(context):
    context.Message("Checking if host is Linux... ")
    import string
    uname = os.uname()
    if string.lower(uname[ 0 ]) == "linux":
        context.Result(1)
        return 1
    context.Result(0)
    return 0

def CheckFBCon(context):
    context.Message("Checking for fbcon support... ")
    ret = context.TryLink("""
        #include <linux/fb.h>
        int main(){
            int x = FB_SYNC_ON_GREEN;
        }
        """, ".c");
    context.Result(ret)
    return ret

def CheckSVGALibVersion(context):
    context.Message("Checking for SVGAlib version... ")
    ret = context.TryLink("""
        #include <vga.h>
        int main(){
            int x = vga_version; 
            x++;
        }
        """, ".c");
    context.Result(ret)
    return ret

def CheckConstructor(context):
    context.Message("Checking if constructors are supported... ")
    ret, message = context.TryRun("""
        static int notsupported = 1;
        void test_ctor (void) __attribute__ ((constructor));
        void test_ctor (void) { notsupported = 0; }
        int main (void) {
            return (notsupported);
        }
        """, ".c")
    context.Result(ret)
    return ret

def AlsaVersion(context):
    context.Message("Checking ALSA version... ")
    ret = context.TryLink("""
        #include <sys/asoundlib.h>
                  #if (SND_LIB_MAJOR > 0) || (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9 && SND_LIB_SUBMINOR >= 1)
                  #else
                  #error
                  #endif
          int main(){}
          """, ".c");
    if ret:
        context.Result(ret)
        return 9
    ret = context.TryLink("""
        #include <sys/asoundlib.h>
                  #if SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 5
                  #else
                  #error
                  #endif
          """, ".c");
    context.Result(ret)
    if ret:
        return 5
    return ret

def CheckALSADigi(context):
    context.Message("Checking for supported ALSA version for digital sound... ")
    ret = context.TryLink("""
        #include <sys/asoundlib.h>
                  #if (SND_LIB_MAJOR > 0) || (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9 && SND_LIB_SUBMINOR >= 1)
                  #else
                  #error
                  #endif
          int main(){}
          """, ".c");
    if ret:
        context.Result(1)
        return 1
    ret = context.TryLink("""
        #include <sys/asoundlib.h>
                  #if SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 5
                  #else
                  #error
                  #endif
          """, ".c");
    context.Result(1)
    if ret:
        return 1
    return ret

def CheckXCursor(context):
    context.Message("Checking for XCursor... ")
    tmpEnv = context.env.Copy()
    context.env.Append(LIBS = 'Xcursor')
    ret = context.TryLink("""
        #include <X11/Xlib.h>
        #include <X11/Xcursor/Xcursor.h>

        int main(){
            XcursorImage *xcursor_image;
            XcursorImageLoadCursor(0, xcursor_image);
            XcursorSupportsARGB(0);
        }
        """, ".c");
    if not ret:
        context.sconf.env = tmpEnv
    context.Result(ret)
    return ret

def CheckALSAMidi(context):
    return CheckALSADigi(context)

def CheckARTSDigi(context):
    context.Message("Checking for ARTS digi... ")

    ret = context.TryAction('artsc-config --version')[0]
    if ret:
        tmpEnv = context.env.Copy()
        context.env.ParseConfig('artsc-config --libs --cflags')
        ret = context.TryLink("""
            #include <artsc.h>
            int main(){
                arts_init();
            }
            """, ".c");
        if not ret:
            context.sconf.env = tmpEnv
    context.Result(ret)
    return ret

def CheckESDDigi(context):
    context.Message("Checking for ESD... ")

    ret = context.TryAction('esd-config --version')[0]
    if ret:
        tmpEnv = context.env.Copy()
        context.env.ParseConfig('esd-config --libs --cflags')
        ret = context.TryLink("""
            #include <esd.h>
            int main(){
                esd_open_sound(0);
            }
            """, ".c");
        if not ret:
            context.sconf.env = tmpEnv
    context.Result(ret)
    return ret

def CheckJackDigi(context):
    context.Message("Checking for JACK... ")

    ret = context.TryAction('pkg-config --libs jack')[0]
    if ret:
	tmpEnv = context.env.Copy()
	context.env.ParseConfig('pkg-config --libs jack --cflags jack')
	ret = context.TryLink("""
	    #include <jack/jack.h>
	    int main(){
		jack_client_new(0);
	    }
	    """, ".c");
	if not ret:
	    context.sconf.env = tmpEnv
    context.Result(ret)
    return ret

def CheckForX(context):

    class NoX(Exception):
        pass

    try:
        ret = context.sconf.CheckHeader('X11/X.h')
        if not ret:
            raise NoX
        ret = context.sconf.CheckLib('X11', 'XOpenDisplay')
        if not ret:
            raise NoX
    except NoX:
        oldEnv = context.env.Copy()
        context.env.Append(CPPPATH = "/usr/X11R6/include")
        context.env.Append(LIBPATH = "/usr/X11R6/lib")
        import SCons.SConf
        try:
            ret = context.sconf.CheckHeader('X11/X.h')
            if not ret:
                raise NoX
            ret = context.sconf.CheckLib('X11', 'XOpenDisplay')
            if not ret:
                raise NoX
        except NoX:
            context.sconf.env = oldEnv
    context.Message("Checking if X11 found...")
    context.Result(ret)
    return ret

def CheckForGLX(context):

    result = context.sconf.CheckHeader('GL/glx.h') and\
        context.sconf.CheckHeader('GL/gl.h') and\
        context.sconf.CheckLib('GL', 'glXCreateWindow')

    context.Message("Checking if GLX found...")
    context.Result(result)
    return result

def CheckOSSDigi(context):
    result = False
    headers = [ 'soundcard.h', 'sys/soundcard.h', 'machine/soundcard.h', 'linux/soundcard.h' ]
    for i in headers:
        if context.sconf.CheckHeader(i):
            result = True
    context.Message("Checking for OSS...")
    context.Result(result)
    return result

def CheckOSSMidi(context):
    result = False
    headers = [ 'soundcard.h', 'sys/soundcard.h', 'machine/soundcard.h', 'linux/soundcard.h' ]
    for i in headers:
        if context.sconf.CheckHeader(i):
            if context.sconf.TryLink("""
                #include <%s>
                int main(){
                    return SNDCTL_SEQ_NRSYNTHS;
                } """ % i, ".c"):
                result = True
    context.Message("Checking for OSS MIDI...")
    context.Result(result)
    return result

def CheckMapFailed(context):
    context.Message("Checking for MAP_FAILED... ")
    ret = context.TryCompile("""
        #include <unistd.h>
        #include <sys/mman.h>

        int test_mmap_failed (void *addr){
            return (addr == MAP_FAILED);
        }

        int main(){
        }
        """, ".c")
    context.Result(ret)
    if ret:
        ret = 0
    return ret


