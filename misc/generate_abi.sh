#!/bin/bash
set -e

build_dir="${1}"

if [[ -z "${build_dir}" ]]
then
    echo "Usage: $0 <build-dir>"
    exit 1
fi

if ! command -v abidw > /dev/null 2>&1
then
    echo "abidw not found"
    exit 1
fi

library_paths=("${build_dir}"/lib/*.so)
abidw_args=(
   --suppressions misc/suppressions.abignore
   --exported-interfaces-only
)

for library_path in "${library_paths[@]}";
do
   library_filename=$(basename "${library_path}")
   library="${library_filename%.*}"
   out_path="misc/${library}_abi.xml"
   echo Generating $out_path
   abidw "${abidw_args[@]}" "${library_path}" --out-file "${out_path}"
   sed -i "s@$(pwd)/@@g" "${out_path}"
done
