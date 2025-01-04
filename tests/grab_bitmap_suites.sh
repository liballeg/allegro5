# bmp suite 1
wget -N https://liballeg.org/files/bmpsuite/bmpsuite.zip
unzip -u -d bmpsuite bmpsuite.zip

# bmp suite 2
wget -N https://liballeg.org/files/bmpsuite/bmptestsuite-0.9.zip
unzip -u bmptestsuite-0.9.zip

# bmp suite 3
wget -N -P wvnet https://liballeg.org/files/bmpsuite/wvnet/test32bfv4.bmp
wget -N -P wvnet https://liballeg.org/files/bmpsuite/wvnet/test32v5.bmp
wget -N -P wvnet https://liballeg.org/files/bmpsuite/wvnet/trans.bmp

# bmp suite 4
wget -N https://liballeg.org/files/bmpsuite/bmpsuite-2.4.zip
unzip -u bmpsuite-2.4.zip
mv bmpsuite-2.4 bmpsuite2
