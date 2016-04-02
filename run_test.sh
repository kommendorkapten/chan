#!/bin/sh

set -u 
set -e

LD_LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH

ccs="gcc clang"

if [ `uname -s` = SunOS ]; then
    ccs="c99 ${ccs}"
fi

for cc in ${ccs}; do
    if command -v ${cc} >/dev/null; then
        # POLL
        gmake clean
        CC=${cc} EVENT_M=poll gmake -e bin/test
        ./bin/test

        # SELECT
        gmake clean
        CC=${cc} EVENT_M=select gmake -e bin/test
        ./bin/test
    fi
done
