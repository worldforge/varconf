#! /bin/sh

rm -f config.cache

echo aclocal...
(aclocal --version) < /dev/null > /dev/null 2>&1 || {
    echo aclocal not found
    exit 1
}
aclocal $ACLOCAL_FLAGS

echo autoheader...
(autoheader --version) < /dev/null > /dev/null 2>&1 || {
    echo autoheader not found
    exit 1
}    
autoheader

echo libtoolize...
LIBTOOLIZE=libtoolize
(libtoolize --version) < /dev/null > /dev/null 2>&1 || {
    echo libtoolize not found
    (glibtoolize --version) < /dev/null > /dev/null 2>&1 || {
        echo gliboolize not found
        exit 1
    }
    LIBTOOLIZE=glibtoolize
}
$LIBTOOLIZE --automake --copy --force

echo automake...
(automake --version) < /dev/null > /dev/null 2>&1 || {
    echo automake not found
    exit 1
}
automake --add-missing --copy --gnu

echo autoconf...
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo autoconf not found
    exit 1
}
autoconf

echo Ready to configure

exit 0
