#!/usr/bin/perl

=pod

=head1 DESCRIPTION

This script is called from misc/deplib.sh to I<properly> grab dependencies for a
list of source files. It assumes its being run from the base allegro directory.

=cut

use warnings;
use strict;
use File::Spec;

# we assume we are running from the allegro root dir
my $inc_dir = 'include';
my $file = $ARGV[0];
our %deps;

grab_deps($file);

print " ", join(" ", keys %deps);

sub grab_deps {
	my $gpath = shift();
	open my $fh, $gpath or return; #die "failed to open $path: $!";
	for my $line (<$fh>) {
		if($line =~ /#\s*include\s+"([^"]+)"/) {
			my $file = $1;
			my $path = File::Spec->catfile($inc_dir, $file);
			# this override could probably be done in a better way,
			#  possibly by checking if the given include exists in the same
			#  directory as the source file,
			#  but thats only a problem in the assembly as of now.
			if($file =~ /\.s$/ || $file =~ /\.inc$/) {
				my ($volume,$dirs,$file) = File::Spec->splitpath( $gpath );
				my $rpath = File::Spec->catfile($dirs, $file);
				$deps{$rpath} = 1;

				grab_deps($rpath);
			}
			elsif(!exists $deps{$file} && $file =~ /asmdef/) {
				$deps{$file} = 1;
				grab_deps($file);
			}
			elsif(!exists $deps{$path} && -f $path) {
				$deps{$path} = 1;
				grab_deps($path);
			}
		}
	}
}
