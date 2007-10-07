# removes MSVC project files which do not need to be distributed

IFS="
"
# this code looks for trouble
for FILE in `find -mindepth 2 -name \*.plg`;
	do echo $FILE
	rm -fr "$FILE";
done;
rm msvc6.opt
rm msvc6.ncb
