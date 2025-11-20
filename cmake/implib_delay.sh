#!/usr/bin/env bash
# Create a delay-loaded import library in CWD for a given -l<name>
#
# Usage: ./implib_delay.sh -lgdiplus
set -euo pipefail

err() { printf '%s\n' "implib_delay.sh: $*" >&2; }
die() { err "$@"; exit 1; }

[[ $# -eq 1 ]] || die "exactly one argument required (e.g. -lgdiplus)"
arg="$1"

libname=""
case "$arg" in
    -l*) libname="${arg#-l}" ;;
    lib*.a) libname="${arg#lib}"; libname="${libname%.a}" ;;
    *) libname="$arg" ;;
esac
[[ -n "$libname" ]] || die "could not parse library name from '$arg'"

if [[ -f "$arg" ]]; then
    implib="$arg"
    # Derive libname from provided file path (e.g. /path/libfoo.a -> foo)
    bn="$(basename "$implib")"
    t="${bn#lib}"
    t="${t%.dll.a}"
    t="${t%.a}"
    [[ -n "$t" && "$t" != "$bn" ]] || die "could not derive library name from file '$implib'"
    libname="$t"
else
    implib="$("${CC:-gcc}" -print-file-name="lib${libname}.a")"
    [[ "$implib" != "lib${libname}.a" ]] || die "import library lib${libname}.a not found in compiler search paths"
fi

implib_dir="$(dirname "$implib")"
out_delaylib="lib${libname}_delay.a"

if [[ ! -f "$out_delaylib" ]]; then
    dllname="$(dlltool --identify "$implib" 2>/dev/null | tr -d '\r' | head -n1 || true)"
    [[ -n "$dllname" ]] || exit 0  # Not a DLL import library; nothing to do
    dllpath="$(which "$dllname" 2>/dev/null | tr -d '\r' | head -n1 || true)"
    [[ -n "$dllpath" && -f "$dllpath" ]] || die "could not locate $dllname on disk"
    gendef "$dllpath" > /dev/null
    def_file="${dllname%.dll}.def"
    [[ -f "$def_file" ]] || die "gendef did not produce $def_file"
    dlltool -k -d "$def_file" -y "$out_delaylib"
    [[ -f "$out_delaylib" ]] || die "failed to create delay import library: $out_delaylib"
fi

printf -- "-l%s_delay\n" "$libname"
# echo "$out_delaylib"
