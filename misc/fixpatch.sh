#! /bin/sh
#
# Updates timestamps in order, to avoid rebuilding configuration files.
#

touch configure.in
touch makefile.in
touch aclocal.m4
touch acconfig.h
touch include/allegro/platform/alunixac.hin
touch stamp-h.in
touch configure

