#!/bin/bash
#set -e

build_dir="${1}"

if [[ -z "${build_dir}" ]]
then
    echo "Usage: $0 <build-dir>"
    exit 1
fi

if ! command -v abidiff > /dev/null 2>&1
then
    echo "abidiff not found"
    exit 1
fi

library_paths=("${build_dir}"/lib/*.so)
abidiff_args=(
   --suppressions misc/suppressions.abignore
   --leaf-changes-only
)

for library_path in "${library_paths[@]}";
do
   library_filename=$(basename "${library_path}")
   library="${library_filename%.*}"
   abi_path="misc/${library}_abi.xml"
   echo Comparing ${library}
   header=$(cat <<HEADER
================================
Comparing ${library}
================================

HEADER
   )
   report=$(abidiff "${abidiff_args[@]}" "${abi_path}" "${library_path}")

   printf "%s\n%s" "${header}" "${report}" | less
done
