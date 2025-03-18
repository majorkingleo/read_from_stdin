#!/usr/bin/env bash


git log > ChangeLog
touch NEWS
touch README
touch AUTHORS

aclocal
automake --add-missing
automake -f
autoconf -f

./configure --host=i686-w64-mingw32 $*

SYSROOT=/usr/i686-w64-mingw32/sys-root/mingw/bin/

cp -u ${SYSROOT}/iconv.dll .
cp -u ${SYSROOT}/libgcc_s_sjlj-1.dll .
cp -u ${SYSROOT}/libstdc++-6.dll .
cp -u ${SYSROOT}/libwinpthread-1.dll .
