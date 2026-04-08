#!/bin/sh
set -e

cd src/external/ecm

if [ ! -f configure ]; then
    autoreconf -i
fi

# Fix endian functions on macOS
sed -i.bak 's/htole32/ /g' resume.c
sed -i.bak 's/htole64/ /g' resume.c
sed -i.bak 's/le32toh/ /g' resume.c
sed -i.bak 's/le64toh/ /g' resume.c

sed -i.bak 's/htole32/ /g' torsions.c
sed -i.bak 's/htole64/ /g' torsions.c
sed -i.bak 's/le32toh/ /g' torsions.c
sed -i.bak 's/le64toh/ /g' torsions.c

MY_CFLAGS="-O2"

if [ -d /opt/homebrew/include ]; then
    ./configure --with-gmp=/opt/homebrew CFLAGS="$MY_CFLAGS -I/opt/homebrew/include" LDFLAGS="-L/opt/homebrew/lib"
elif [ -d /usr/local/include ]; then
    ./configure --with-gmp=/usr/local CFLAGS="$MY_CFLAGS -I/usr/local/include" LDFLAGS="-L/usr/local/lib"
else
    ./configure CFLAGS="$MY_CFLAGS"
fi

make -j4
