#!/bin/sh
#autoreconf -fi
aclocal -I aclocal
autoheader
automake --gnu --add-missing
autoconf
