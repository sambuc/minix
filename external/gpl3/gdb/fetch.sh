#!/bin/sh

# Make sure we're in our directory (i.e., where this shell script is)
echo $0
cd `dirname $0`

# Configure fetch method
URL="http://www.minix3.org/distfiles-minix/gdb-7.3.1.tar.bz2"
BACKUP_URL="http://ftp.gnu.org/gnu/gdb/gdb-7.3.1.tar.bz2"
FETCH=wget
which curl >/dev/null
if [ $? -eq 0 ]; then
	FETCH="curl -O -f"
fi

# Fetch sources if not available
if [ ! -d dist ];
then
	if [ ! -f gdb-7.3.1.tar.bz2 ]; then
		$FETCH $URL
		if [ $? -ne 0 ]; then
			$FETCH $BACKUP_URL
		fi
	fi

	tar -oxjf gdb-7.3.1.tar.bz2 && \
	mv gdb-7.3.1 dist && \
	cd dist && \
	cat ../patches/* | patch -p1 && \
	cp ../files/i386minix-nat.c gdb && \
	cp ../files/i386minix-tdep.c gdb && \
	cp ../files/minix-nat.c gdb && \
	cp ../files/minix-nat.h gdb && \
	cp ../files/minix.mh gdb/config/i386 && \
	cp ../files/nm-minix.h gdb/config/i386
fi
