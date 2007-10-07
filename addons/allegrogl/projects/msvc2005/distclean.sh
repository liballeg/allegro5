# removes MSVC project files which do not need to be distributed

IFS="
"
# this code looks for trouble
for FILE in `find -mindepth 2 -not -name \*.vcproj -and -not -name \*.sln -and -not -wholename \*.svn\*`;
	do echo $FILE
	rm -fr "$FILE";
done;
rm msvc2005.ncb
rm msvc2005.suo
