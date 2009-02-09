#!/usr/bin/perl

=pod

=head1 DESCRIPTION

This script checks to see if there are any files that exist in the
main source dir, and aren't in makefile.lst. It currently only checks src/
against ALLEGRO_SRC_FILES, though it can easily be expanded to the platform dirs
as well. It's more of a handy utility for someone that might want to keep
the makefiles up to do date, in case others have been lax.

=cut

use strict;
use warnings;
use File::Spec::Functions;

my $path = $ARGV[0];

if(!defined $path || !-d $path) {
	print "usage $0 <path to allegro root>\n";
	exit 0;
}

my %main_sources;
my $srcdir = catdir $path, "src";
opendir DIR, $srcdir or die "failed to scan $path: $!\n";
for my $ent (readdir DIR) {
	my $fpath = catfile('src',$ent);
	next if !-f $fpath;
	if($ent =~ /\.c$/) {
		$main_sources{$fpath} = 1;
	}
}
closedir DIR;

my $lstdata;
open FH, catfile($path,"makefile.lst") or die "failed to open makefile.lst: $!";
	$lstdata = join '', <FH>;
close FH;

$lstdata =~ s/\\\r?\n//sg;
$lstdata =~ s/[ \t]+/ /g;
my @lines = split /\r?\n/, $lstdata;

my %lstdata;
for my $line (@lines) {
	chomp $line;
	next if $line =~ /^#/;
	next if $line =~ /^\s*$/;
	my ($name, $value) = split /\s*=\s*/, $line;
	$lstdata{$name} = $value;
#	print $name."\n";
}

#print $lstdata{"ALLEGRO_SRC_FILES"}."\n";

my %srcfiles = map { $_ => 1 } split /\s/, $lstdata{"ALLEGRO_SRC_FILES"};
my @missing_sources;
for my $file (keys %main_sources) {
	if(!exists $srcfiles{$file}) {
		push @missing_sources, $file;
	}
}

if(scalar(@missing_sources)) {
	print "warning, the following files exist, but aren't in ALLEGRO_SRC_FILES: "
		.join(" ",@missing_sources)."\n";
}
else {
	print "No problems with ALLEGRO_SRC_FILES.\n";
}