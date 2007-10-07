#! /usr/bin/perl -w
#
#  Generates djgpp/Unix/Mingw32 makefiles, Watcom and MSVC batch files,
#  and an MSVC version 6 project file (.dsp) for building portable
#  libraries, reading info about them from the buildlib.in file.
#
#  By George Foot, October 2000.
#
#  Based on `builder.pl' by Shawn Hargreaves, December 1999.


# my filename
$me = $0;
$me =~ s,.*/,,;

# builder file directory
$build = $0;
$build =~ s,(.*)/.*,$1,;

# project configuration file
$in = "$build/$me";
$in =~ s/(.*)\.[^\.]*/$1.in/;

print "Reading $in\n";
open FILE, "$in" or die "\nError: can't open $in:\n   $!";

while (<FILE>) {
	s/#.*//;

	if (/\s*([^=]*\S)\s*=\s*(.*)/) {
		$param{$1} = $2;
	}
}

close FILE;

die "Error: NAME not set in $in" unless ($param{"NAME"});
die "Error: OBJDIR not set in $in" unless ($param{"OBJDIR"});
die "Error: LIBDIR not set in $in" unless ($param{"LIBDIR"});



# check whether we should clean up a bit
if (defined $ARGV[0] and $ARGV[0] eq "clean") {
	print "Removing build files...\n";
	unlink "$build/makefile.all", "$build/makefile.dj", "$build/makefile.uni",
		"$build/makefile.mgw", "$build/clean.bat", "$build/$param{'NAME'}.dsp",
		"$build/msvcmake.bat", "$build/watmake.bat", "fixdjgpp.bat",
		"fixwat.bat", "fixunix.sh", "fixmsvc.bat", "fixmingw.bat", "makefile",
		"makefile.all", "clean.bat", "msvcmake.bat", "$param{'NAME'}.dsp";
	exit;
}



# import package settings
foreach $package (split (" ", $param{"PACKAGES"})) {
	$in = "$build/$package.pkg";
	print "Reading $in\n";
	open FILE, "$in" or die "\nError: can't open $in:\n   $!";

	while (<FILE>) {
		s/#.*//;

		if (/\s*([^=]*\S)\s*\+=\s*(.*)/) {
			$param{$1} .= " $2";
		}
		elsif (/\s*([^=]*\S)\s*=\s*(.*)/) {
			$param{$1} = $2;
		}
	}
	close FILE;
}


# convert obj and bin paths to pretty formats
$dosobj = $unixobj = $param{"OBJDIR"};
$doslib = $unixlib = $param{"LIBDIR"};

# get slashes the right way around
$dosobj =~ s/\//\\/g;
$doslib =~ s/\//\\/g;

$unixobj =~ s/\\/\//g;
$unixlib =~ s/\\/\//g;

# strip trailing slashes
$dosobj =~ s/\\$//;
$doslib =~ s/\\$//;

$unixobj =~ s/\/$//;
$unixlib =~ s/\/$//;


print "\n";


# scans a source file for dependency information
sub get_dependencies
{
	my $filename = shift;

	return if ($depends{$filename});

	# scan the file for include statements
	open FILE, $filename or die "\nError opening $filename:\n   $!";

	my @deps = ();

	while (<FILE>) {
		# strip comments
		s/\/\/.*//;
		s/\/\*.*\*\///g;

		if (/^\s*#include\s*[<"](.*)[>"]/) {
			push @deps, $1;
		}
	}

	close FILE;

	# build the dependency list
	$depends{$filename}{$filename} = 1;

	for my $header (@deps) {
		my $head = $header;
		$head =~ s/\\/\//g;

		if ($filename =~ /(.*[\\\/])/) {
			$head = $1 . $head;
		}

		if (-f $head) {
			# recurse into a header
			$headers{$head} = 1;
			$depends{$filename}{$head} = 1;

			get_dependencies($head);

			for (keys %{$depends{$head}}) {
				$depends{$filename}{$_} = 1;
			}
		}
		else {
			$unresolved{$header} = 1;
		}
	}
}



# scan for source files
sub scan_directory
{
	my $dir = shift;

	for my $filename (<$dir*>) {
		if (-d $filename) {
			# recurse into a directory
			scan_directory("$filename/");
		}
		elsif ($filename =~ /\.(c|cpp)$/) {
			# got a source file
			print "   $filename\n";
			$cplusplus = 1 if ($1 eq "cpp");
			$sources{$filename} = 1;
			get_dependencies($filename);
		}
	}
}



# initiate the recursive directory scan
print "Looking for source files...\n";

if (exists $param{"SRCDIR"}) {
	scan_directory($param{"SRCDIR"});
} else {
	scan_directory("");
}
print "\n";



# warn about any unresolved headers
%standard_headers = (
	"assert.h" => 1,
	"ctype.h" => 1,
	"errno.h" => 1,
	"fcntl.h" => 1,
	"limits.h" => 1,
	"locale.h" => 1,
	"malloc.h" => 1,
	"math.h" => 1,
	"memory.h" => 1,
	"search.h" => 1,
	"setjmp.h" => 1,
	"signal.h" => 1,
	"stdarg.h" => 1,
	"stdio.h" => 1,
	"stdlib.h" => 1,
	"string.h" => 1,
	"sys/stat.h" => 1,
	"sys/timeb.h" => 1,
	"sys/types.h" => 1,
	"time.h" => 1,
);

if ($unresolved) {
	print "Warning: project uses non-standard header files:\n";

	for (sort keys %unresolved) {
		print "Warning: program uses non-standard header $_\n" unless ($standard_headers{$_}) or ($param{"STDHDRS"} =~ /$_/);
	}
	print "\n";
}



# check if we have info for the djgpp output
if (!exists $param{"DJGPP_RELEASE_CFLAGS"}) {
	print "DJGPP_RELEASE_CFLAGS parameter not set: skipping djgpp output files\n";
}
elsif (!exists $param{"DJGPP_DEBUG_CFLAGS"}) {
	print "DJGPP_DEBUG_CFLAGS parameter not set: skipping djgpp output files\n";
}
elsif (!exists $param{"DJGPP_RELEASE_LFLAGS"}) {
	print "DJGPP_RELEASE_LFLAGS parameter not set: skipping djgpp output files\n";
}
elsif (!exists $param{"DJGPP_DEBUG_LFLAGS"}) {
	print "DJGPP_DEBUG_LFLAGS parameter not set: skipping djgpp output files\n";
}
else {
	$got_djgpp = 1;
}



# check if we have info for the Unix output
if (!exists $param{"UNIX_RELEASE_CFLAGS"}) {
	print "UNIX_RELEASE_CFLAGS parameter not set: skipping Unix output files\n";
}
elsif (!exists $param{"UNIX_DEBUG_CFLAGS"}) {
	print "UNIX_DEBUG_CFLAGS parameter not set: skipping Unix output files\n";
}
elsif (!exists $param{"UNIX_RELEASE_LFLAGS"}) {
	print "UNIX_RELEASE_LFLAGS parameter not set: skipping Unix output files\n";
}
elsif (!exists $param{"UNIX_DEBUG_LFLAGS"}) {
	print "UNIX_DEBUG_LFLAGS parameter not set: skipping Unix output files\n";
}
else {
	$got_unix = 1;
}



# check if we have info for the Mingw32 output
if (!exists $param{"MINGW_RELEASE_CFLAGS"}) {
	print "MINGW_RELEASE_CFLAGS parameter not set: skipping Mingw32 output files\n";
}
elsif (!exists $param{"MINGW_DEBUG_CFLAGS"}) {
	print "MINGW_DEBUG_CFLAGS parameter not set: skipping Mingw32 output files\n";
}
elsif (!exists $param{"MINGW_RELEASE_LFLAGS"}) {
	print "MINGW_RELEASE_LFLAGS parameter not set: skipping Mingw32 output files\n";
}
elsif (!exists $param{"MINGW_DEBUG_LFLAGS"}) {
	print "MINGW_DEBUG_LFLAGS parameter not set: skipping Mingw32 output files\n";
}
else {
	$got_mingw = 1;
}



# check if we have info for the Watcom output
if (!exists $param{"WATCOM_LFLAGS"}) {
	print "WATCOM_LFLAGS parameter not set: skipping Watcom output files\n";
}
elsif (!exists $param{"WATCOM_CFLAGS"}) {
	print "WATCOM_CFLAGS parameter not set: skipping Watcom output files\n";
}
else {
	$got_watcom = 1;
}



# check if we have info for the MSVC output
if (!exists $param{"MSVC_RELEASE_LFLAGS"}) {
   print "MSVC_RELEASE_LFLAGS parameter not set: skipping MSVC output files\n";
}
elsif (!exists $param{"MSVC_DEBUG_LFLAGS"}) {
   print "MSVC_DEBUG_LFLAGS parameter not set: skipping MSVC output files\n";
}
elsif (!exists $param{"MSVC_RELEASE_CFLAGS"}) {
   print "MSVC_RELEASE_CFLAGS parameter not set: skipping MSVC output files\n";
}
elsif (!exists $param{"MSVC_DEBUG_CFLAGS"}) {
   print "MSVC_DEBUG_CFLAGS parameter not set: skipping MSVC output files\n";
}
else {
	$got_msvc = 1;
}



# check for include directories
if (exists $param{"INCDIR"}) {
	$param{DJGPP_RELEASE_CFLAGS} .= " -I ".$param{"INCDIR"};
	$param{DJGPP_DEBUG_CFLAGS} .= " -I ".$param{"INCDIR"};

	$param{UNIX_RELEASE_CFLAGS} .= " -I ".$param{"INCDIR"};
	$param{UNIX_DEBUG_CFLAGS} .= " -I ".$param{"INCDIR"};

	$param{MINGW_RELEASE_CFLAGS} .= " -I ".$param{"INCDIR"};
	$param{MINGW_DEBUG_CFLAGS} .= " -I ".$param{"INCDIR"};

	$param{MSVC_RELEASE_CFLAGS} .= ' /I "'.$param{"INCDIR"}.'"';
	$param{MSVC_DEBUG_CFLAGS} .= ' /I"'.$param{"INCDIR"}.'"';
}



# add platform defines
$param{DJGPP_RELEASE_CFLAGS} .= " -D TARGET_DJGPP";
$param{DJGPP_DEBUG_CFLAGS} .= " -D TARGET_DJGPP";

$param{UNIX_RELEASE_CFLAGS} .= " -D TARGET_UNIX";
$param{UNIX_DEBUG_CFLAGS} .= " -D TARGET_UNIX";

$param{MINGW_RELEASE_CFLAGS} .= " -D TARGET_MINGW";
$param{MINGW_DEBUG_CFLAGS} .= " -D TARGET_MINGW";

$param{MSVC_RELEASE_CFLAGS} .= " -D TARGET_MSVC";
$param{MSVC_DEBUG_CFLAGS} .= " -D TARGET_MSVC";



# write out GNU makefiles for djgpp, Unix, and Mingw32
if ($got_djgpp or $got_unix or $got_mingw) {
	print "Writing makefiles...\n  ";

	if ($got_djgpp) {
		print " makefile.dj";
		open FILE, "> $build/makefile.dj" or die "\nError creating makefile.dj:\n   $!";
		print FILE "# generated by $me: makefile for djgpp\n\n";

		print FILE "CC = " . ($cplusplus ? "gxx" : "gcc") . "\n";

		print FILE <<EOF;
PLATFORM=djgpp
EXE = .exe
RELEASE_CFLAGS = $param{DJGPP_RELEASE_CFLAGS}
DEBUG_CFLAGS = $param{DJGPP_DEBUG_CFLAGS}
RELEASE_LFLAGS = $param{DJGPP_RELEASE_LFLAGS}
DEBUG_LFLAGS = $param{DJGPP_DEBUG_LFLAGS}

include makefile.all
EOF
		close FILE;
	}

	if ($got_unix) {
		print " makefile.uni";
		open FILE, "> $build/makefile.uni" or die "\nError creating makefile.uni:\n   $!";
		print FILE "# generated by $me: makefile for Unix\n\n";

		print FILE "CC = " . ($cplusplus ? "g++" : "gcc") . "\n";

		print FILE <<EOF;
PLATFORM=unix
EXE =
RELEASE_CFLAGS = $param{UNIX_RELEASE_CFLAGS}
DEBUG_CFLAGS = $param{UNIX_DEBUG_CFLAGS}
RELEASE_LFLAGS = $param{UNIX_RELEASE_LFLAGS}
DEBUG_LFLAGS = $param{UNIX_DEBUG_LFLAGS}

include makefile.all
EOF
		close FILE;
	}

	if ($got_mingw) {
		print " makefile.mgw";
		open FILE, "> $build/makefile.mgw" or die "\nError creating makefile.mgw:\n   $!";
		print FILE "# generated by $me: makefile for Mingw32\n\n";

		print FILE "CC = " . ($cplusplus ? "g++" : "gcc") . "\n";

		print FILE <<EOF;
PLATFORM=mingw
EXE = .exe
RELEASE_CFLAGS = $param{MINGW_RELEASE_CFLAGS}
DEBUG_CFLAGS = $param{MINGW_DEBUG_CFLAGS}
RELEASE_LFLAGS = $param{MINGW_RELEASE_LFLAGS}
DEBUG_LFLAGS = $param{MINGW_DEBUG_LFLAGS}

EOF

		$winicon = "";

		if ($param{WINICON}) {
			$winicon = $param{WINICON};
			$winicon =~ s/\\/\//g;
			print FILE "OBJS += $unixobj/\$(PLATFORM)/icon.a\n\n";
		}
		print FILE <<EOF;
include makefile.all

$unixobj/\$(PLATFORM)/icon.a: $winicon
	echo MYICON ICON \$^ | windres -o \$@
EOF
		close FILE;
	}

	print " makefile.all";
	open FILE, "> $build/makefile.all" or die "\nError creating makefile.all:\n   $!";

	print FILE <<EOF;
# generated by $me: shared between djgpp, Unix and Mingw32

.PHONY: all lib clean debug release

all: lib

lib: debug release

debug: $unixlib/\$(PLATFORM)/lib$param{NAME}d.a
release: $unixlib/\$(PLATFORM)/lib$param{NAME}.a

OBJS += \\
EOF

	$first = 1;

	for (sort keys %sources) {
		$filename = $_;
		$filename =~ /([^\/.]+)\./;
		$objname = "$1.o";

		print FILE " \\\n" unless ($first);
		print FILE "\t$objname";

		$first = 0;
	}

	print FILE <<EOF;
\n
$unixlib/\$(PLATFORM)/lib$param{NAME}d.a: \$(addprefix $unixobj/\$(PLATFORM)/$param{NAME}d/,\$(OBJS))
\tar rs \$@ \$^
$unixlib/\$(PLATFORM)/lib$param{NAME}.a:  \$(addprefix $unixobj/\$(PLATFORM)/$param{NAME}/,\$(OBJS))
\tar rs \$@ \$^

EOF

	for (sort keys %sources) {
		$filename = $_;
		$filename =~ /([^\/.]+)\./;
		$objname = "$1.o";

		print FILE "$unixobj/\$(PLATFORM)/$param{NAME}d/$objname: " . (join ' ', sort keys %{$depends{$_}}) . "\n";
		print FILE "\t\$(CC) \$(DEBUG_CFLAGS) -c $filename -o \$@\n\n";
		print FILE "$unixobj/\$(PLATFORM)/$param{NAME}/$objname: " . (join ' ', sort keys %{$depends{$_}}) . "\n";
		print FILE "\t\$(CC) \$(RELEASE_CFLAGS) -c $filename -o \$@\n\n";
	}

	print FILE <<EOF;
CLEAN_FILES = \\
\t$param{"NAME"}.dsw \\
\t$param{"NAME"}.ncb \\
\t$param{"NAME"}.opt \\
\t$param{"NAME"}.plg \\
\t*.pch \\
\t*.idb \\
\t*.err \\
\t$unixobj/msvc/*/*.pch \\
\t$unixobj/msvc/*/*.idb \\
\t$unixobj/msvc/*/*.pdb \\
\t$unixobj/msvc/*/*.obj \\
\t$unixobj/*.o \\
\t$unixobj/*.a \\
\t$unixlib/*/lib$param{"NAME"}d.a \\
\t$unixlib/*/lib$param{"NAME"}.a \\
\t$unixlib/*/$param{"NAME"}d.lib \\
\t$unixlib/*/$param{"NAME"}.lib \\
\t$unixlib/*/$param{"NAME"}ds.lib \\
\t$unixlib/*/$param{"NAME"}s.lib \\
\t$unixlib/msvc/$param{"NAME"}.ilk \\
\t$unixlib/msvc/$param{"NAME"}.pdb \\
\tallegro.log \\
\tcore

clean:
\trm -f \$(CLEAN_FILES)
EOF

	close FILE;
	print "\n";
}



if ($got_watcom) {
	# write a batch file for building with Watcom
	print "Writing watmake.bat\n";

	open FILE, "> $build/watmake.bat" or die "\nError creating watmake.bat:\n   $!";
	binmode FILE;

	print FILE <<EOF;
\@echo off\r
rem generated by $me: compiles using Watcom\r
\r
echo option quiet > _ld.arg\r
echo name $doslib\\$param{NAME}.exe >> _ld.arg\r
EOF

	while ($param{"WATCOM_LFLAGS"} =~ /"([^"]+)"/g) {
		print FILE "echo $1 >> _ld.arg\r\n";
	}

	print FILE "\r\n";

	for (sort keys %sources) {
		$filename = $_;
		$filename =~ s/\//\\/g;

		$filename =~ /([^\\.]+)\./;
		$objname = "$dosobj\\$1.obj";

		print FILE "echo $filename\r\n"; 
		print FILE "wcl386.exe $param{WATCOM_CFLAGS} -c $filename /fo=$objname\r\n";
		print FILE "echo file $objname >> _ld.arg\r\n\r\n";
	}

	print FILE <<EOF;
echo Linking\r
wlink \@_ld.arg\r
del _ld.arg > nul\r
\r
echo Done!\r
EOF

	close FILE;
}



if ($got_msvc) {
	# write a MSVC 6.0 .dsp format project
	print "Writing $param{NAME}.dsp\n";

	open FILE, "> $build/$param{NAME}.dsp" or die "\nError creating $param{NAME}.dsp:\n   $!";

	binmode FILE;
	binmode DATA;

	while (<DATA>) {
		s/\r//g;
		chomp;

		if (/__SOURCES__/) {
			# write list of source files
			for (sort keys %sources) {
				$filename = $_;
				$filename =~ s/\//\\/g;
				print FILE "# Begin Source File\r\n\r\nSOURCE=.\\$filename\r\n# End Source File\r\n";
			}
		}
		elsif (/__HEADERS__/) {
			# write list of header files
			for (sort keys %headers) {
				$filename = $_;
				$filename =~ s/\//\\/g;
				print FILE "# Begin Source File\r\n\r\nSOURCE=.\\$filename\r\n# End Source File\r\n";
			}
		}
		else {
			# change 'untitled' to our project name
			s/untitled/$param{"NAME"}/g;

			# change Intermediate_Dir to our obj directory
			s/__LIBDIR__/$unixlib/g;

			# change Output_Dir to our bin directory
			s/__OBJDIR__/$unixobj/g;

			# change __RLIBS__ and __DLIBS__
			s/__RLIBS__/$param{"MSVC_RELEASE_LFLAGS"}/;
			s/__DLIBS__/$param{"MSVC_DEBUG_LFLAGS"}/;

			# change __RFLAGS__ and __DFLAGS__
			s/__RFLAGS__/$param{"MSVC_RELEASE_CFLAGS"}/;
			s/__DFLAGS__/$param{"MSVC_DEBUG_CFLAGS"}/;

			print FILE "$_\r\n";
		}
	}

	close FILE;

	# write a batch file for building with MSVC
	print "Writing msvcmake.bat\n";

	open FILE, "> $build/msvcmake.bat" or die "\nError creating msvcmake.bat:\n   $!";
	binmode FILE;

	print FILE <<EOF;
\@echo off\r
rem generated by $me: compiles using MSVC\r
\r
if not "%MSVCDIR%" == "" goto got_msvc\r
if not "%MSDEVDIR%" == "" goto got_msvc\r
\r
echo MSVC not installed: you need to run vcvars32.bat\r
goto end\r
\r
:got_msvc\r
\r
EOF

	$first = 1;

	for (sort keys %sources) {
		$filename = $_;
		$filename =~ s/\//\\/g;

		$filename =~ /([^\\.]+)\./;
		$objname = "$1.obj";

		$objpath = "$dosobj\\msvc\\$param{NAME}s\\$objname";
		print FILE "cl.exe /nologo $param{MSVC_RELEASE_CFLAGS} /c $filename /Fo$objpath\r\n";
		print FILE "echo $objpath " . ($first ? ">" : ">>") . " _ld.arg\r\n\r\n";

		$first = 0;
	}

	print FILE <<EOF;
echo Linking\r
link.exe -lib /nologo /out:"$unixlib/msvc/$param{NAME}s.lib" \@_ld.arg\r
del _ld.arg > nul\r
\r
echo Done!\r
:end\r
EOF

	close FILE;
}



# write a 'make clean' batch file
print "Writing clean.bat\n";

open FILE, "> $build/clean.bat" or die "\nError creating clean.bat:\n   $!";
binmode FILE;

print FILE <<EOF;
\@echo off\r
rem generated by $me: cleans out temporary files\r
\r
if exist $param{"NAME"}.dsw del $param{"NAME"}.dsw\r
if exist $param{"NAME"}.ncb del $param{"NAME"}.ncb\r
if exist $param{"NAME"}.opt del $param{"NAME"}.opt\r
if exist $param{"NAME"}.plg del $param{"NAME"}.plg\r
if exist *.pch del *.pch\r
if exist *.idb del *.idb\r
if exist *.err del *.err\r
if exist $dosobj\\msvc\\*\\*.pch del $dosobj\\msvc\\*\\*.pch\r
if exist $dosobj\\msvc\\*\\*.idb del $dosobj\\msvc\\*\\*.idb\r
if exist $dosobj\\msvc\\*\\*.pdb del $dosobj\\msvc\\*\\*.pdb\r
if exist $dosobj\\msvc\\*\\*.obj del $dosobj\\msvc\\*\\*.obj\r
if exist $dosobj\\*\\*\\*.o del $dosobj\\*\\*\\*.o\r
if exist $dosobj\\*\\*\\*.a del $dosobj\\*\\*\\*.a\r
if exist $doslib\\*\\lib$param{"NAME"}d.a del $doslib\\*\\lib$param{"NAME"}d.a\r
if exist $doslib\\*\\lib$param{"NAME"}.a del $doslib\\*\\lib$param{"NAME"}.a\r
if exist $doslib\\*\\$param{"NAME"}d.lib del $doslib\\*\\$param{"NAME"}d.lib\r
if exist $doslib\\*\\$param{"NAME"}.lib del $doslib\\*\\$param{"NAME"}.lib\r
if exist $doslib\\*\\$param{"NAME"}ds.lib del $doslib\\*\\$param{"NAME"}ds.lib\r
if exist $doslib\\*\\$param{"NAME"}s.lib del $doslib\\*\\$param{"NAME"}s.lib\r
if exist $doslib\\msvc\\$param{"NAME"}.ilk del $doslib\\msvc\\$param{"NAME"}.ilk\r
if exist $doslib\\msvc\\$param{"NAME"}.pdb del $doslib\\msvc\\$param{"NAME"}.pdb\r
if exist allegro.log del allegro.log\r
if exist core del core\r
EOF

close FILE;


print "Generating fixup scripts:\n  ";

if ($got_djgpp) {
	print " fixdjgpp.bat";
	open FILE, "> fixdjgpp.bat" or die "\nError creating fixdjgpp.bat:\n   $!";
	binmode FILE;
	print FILE <<EOF;
\@echo off\r
rem generated by $me: configures for building with djgpp\r
if exist clean.bat del clean.bat\r
if exist makefile.* del makefile.*\r
if exist watmake.bat del watmake.bat\r
if exist msvcmake.bat del msvcmake.bat\r
if exist $param{"NAME"}.dsp del $param{"NAME"}.dsp\r
copy $build\\makefile.dj makefile\r
copy $build\\makefile.all\r
copy $build\\clean.bat\r
utod .../*.c .../*.h\r
mkdir lib
mkdir lib\\djgpp
mkdir obj
mkdir obj\\djgpp
mkdir obj\\djgpp\\$param{"NAME"}
mkdir obj\\djgpp\\$param{"NAME"}d
echo Ready for building with djgpp -- type:\r
echo   'make' or 'make lib' for all builds\r
echo   'make debug' for a debugging build\r
echo   'make release' for a release build\r
echo   'make clean' or just 'clean' to tidy up\r
EOF
	close FILE;
}

if ($got_unix) {
	print " fixunix.sh";
	open FILE, "> fixunix.sh" or die "\nError creating fixunix.sh:\n   $!";
	print FILE <<EOF;
#!/bin/sh
# generated by $me: configures for building in Unix
rm -f makefile makefile.* clean.bat watmake.bat msvcmake.bat *.dsp
cp -f $build/makefile.uni makefile
cp -f $build/makefile.all .
find -\\( -name \*.c -o -name \*.h -\\) -exec sh -c "tmpfile=/tmp/.dtou.\$\$;
	tr -d '\\r' < {} > \\\$tmpfile &&
	touch -r {} \\\$tmpfile &&
	mv \\\$tmpfile {}" \\;
mkdir lib lib/unix obj obj/unix obj/unix/$param{"NAME"} obj/unix/$param{"NAME"}d
echo "Ready for building in Unix -- type:"
echo "  'make' or 'make lib' for all builds"
echo "  'make debug' for a debugging build"
echo "  'make release' for a release build"
echo "  'make clean' to tidy up"
EOF
	close FILE;
	chmod 0755, "fixunix.sh";
}

if ($got_mingw) {
	print " fixmingw.bat";
	open FILE, "> fixmingw.bat" or die "\nError creating fixmingw.bat:\n   $!";
	binmode FILE;
	print FILE <<EOF;
\@echo off\r
rem generated by $me: configures for building with Mingw32\r
if exist clean.bat del clean.bat\r
if exist makefile.* del makefile.*\r
if exist watmake.bat del watmake.bat\r
if exist msvcmake.bat del msvcmake.bat\r
if exist $param{"NAME"}.dsp del $param{"NAME"}.dsp\r
copy $build\\makefile.mgw makefile\r
copy $build\\makefile.all\r
copy $build\\clean.bat\r
utod .../*.c .../*.h\r
mkdir lib
mkdir lib\\mingw
mkdir obj
mkdir obj\\mingw
mkdir obj\\mingw\\$param{"NAME"}
mkdir obj\\mingw\\$param{"NAME"}d
echo Ready for building with Mingw32 -- type:\r
echo   'make' or 'make lib' for all builds\r
echo   'make debug' for a debugging build\r
echo   'make release' for a release build\r
echo   'make clean' or just 'clean' to tidy up\r
EOF
	close FILE;
}

if ($got_watcom) {
	print " fixwat.bat";
	open FILE, "> fixwat.bat" or die "\nError creating fixwat.bat:\n   $!";
	binmode FILE;
	print FILE <<EOF;
\@echo off\r
rem generated by $me: configures for building with Watcom\r
if exist clean.bat del clean.bat\r
if exist makefile.* del makefile.*\r
if exist watmake.bat del watmake.bat\r
if exist msvcmake.bat del msvcmake.bat\r
if exist $param{"NAME"}.dsp del $param{"NAME"}.dsp\r
copy $build\\watmake.bat\r
copy $build\\clean.bat\r
utod .../*.c .../*.h\r
echo Ready for building with Watcom -- type:\r
echo   'watmake' for a release build\r
echo   'clean' to tidy up\r
EOF
	close FILE;
}

if ($got_msvc) {
	print " fixmsvc.bat";
	open FILE, "> fixmsvc.bat" or die "\nError creating fixmsvc.bat:\n   $!";
	binmode FILE;
	print FILE <<EOF;
\@echo off\r
rem generated by $me: configures for building with MSVC\r
if exist clean.bat del clean.bat\r
if exist makefile.* del makefile.*\r
if exist watmake.bat del watmake.bat\r
if exist msvcmake.bat del msvcmake.bat\r
if exist $param{"NAME"}.dsp del $param{"NAME"}.dsp\r
copy $build\\msvcmake.bat\r
copy $build\\clean.bat\r
copy $build\\$param{"NAME"}.dsp\r
rem utod .../*.c .../*.h\r
mkdir lib\r
mkdir lib\\msvc\r
mkdir obj\r
mkdir obj\\msvc\r
mkdir obj\\msvc\\$param{"NAME"}\r
mkdir obj\\msvc\\$param{"NAME"}d\r
mkdir obj\\msvc\\$param{"NAME"}s\r
mkdir obj\\msvc\\$param{"NAME"}ds\r
echo Ready for building with MSVC -- type:\r
echo   'msvcmake' for a release build\r
echo   'clean' to tidy up\r
echo or just run the project file $param{"NAME"}.dsp\r
EOF
	close FILE;
}


print "\n\nAll done.  ";

if ($got_djgpp || $got_unix || $got_msvc || $got_mingw || $got_watcom) {
	print "Supported platforms/compilers: ";
	if ($got_djgpp || $got_watcom) {
		print "\n   DOS: ";
		if ($got_djgpp) {
			print "DJGPP";
			if ($got_watcom) {
				print ", ";
			}
		}
		if ($got_watcom) {
			print "Watcom";
		}
	}
	if ($got_unix) {
		print "\n   Unix: GCC";
	}
	if ($got_msvc || $got_mingw) {
		print "\n   Windows: ";
		if ($got_msvc) {
			print "MSVC";
			if ($got_mingw) {
				print ", ";
			}
		}
		if ($got_mingw) {
			print "Mingw32";
		}
	}
	print "\n\n";
	print "Tell your users to run the appropriate fixup scripts to configure for\n";
	print "their platforms.  Have a nice day!\n";
} else {
	print "No platforms are supported.  Check your input script.\n";
}



# Everything from here on is a Microsoft Visual C++ 6.0 format .dsp file,
# as generated by MSVC 6, with a few minor hand edits replacing some data
# with markers. This script generates our custom files by replacing those
# markers with other data as required.

__DATA__
# Microsoft Developer Studio Project File - Name="untitled" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=untitled - Win32 Debug Dynamic
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "untitled.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "untitled.mak" CFG="untitled - Win32 Debug Dynamic"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "untitled - Win32 Release Static" (based on "Win32 (x86) Static Library")
!MESSAGE "untitled - Win32 Debug Static" (based on "Win32 (x86) Static Library")
!MESSAGE "untitled - Win32 Release Dynamic" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "untitled - Win32 Debug Dynamic" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "untitled - Win32 Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "__LIBDIR__/msvc"
# PROP Intermediate_Dir "__OBJDIR__/msvc/untitleds"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /c
# ADD CPP /nologo __RFLAGS__ /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib/msvc/untitleds.lib"

!ELSEIF  "$(CFG)" == "untitled - Win32 Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "__LIBDIR__/msvc"
# PROP Intermediate_Dir "__OBJDIR__/msvc/untitledds"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /c
# ADD CPP /nologo __DFLAGS__ /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib/msvc/untitledds.lib"

!ELSEIF  "$(CFG)" == "untitled - Win32 Release Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "__LIBDIR__/msvc"
# PROP Intermediate_Dir "__OBJDIR__/msvc/untitled"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "untitled_EXPORTS" /c
# ADD CPP /nologo __RFLAGS__ /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "untitled_EXPORTS" /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 __RLIBS__ kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"lib/msvc/untitled.dll"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "untitled - Win32 Debug Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "__LIBDIR__/msvc"
# PROP Intermediate_Dir "__OBJDIR__/msvc/untitledd"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "untitled_EXPORTS" /c
# ADD CPP /nologo __DFLAGS__ /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "untitled_EXPORTS" /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 __DLIBS__ kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"lib/msvc/untitledd.dll" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no /nodefaultlib

!ENDIF 

# Begin Target

# Name "untitled - Win32 Release Static"
# Name "untitled - Win32 Debug Static"
# Name "untitled - Win32 Release Dynamic"
# Name "untitled - Win32 Debug Dynamic"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
__SOURCES__
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
__HEADERS__
# End Group
# End Target
# End Project
