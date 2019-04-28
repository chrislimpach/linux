#!/bin/bash
DRV_NAME=tn40xx
VER=`grep BDX_DRV_VERSION tn40.h | sed -e 's/^[^"]*\"//'  -e 's/"//'`
TAR_FILE=${DRV_NAME}-${VER}
tar zcvf ${TAR_FILE}.tgz -T tar_files --xform s:^:${TAR_FILE}/: 2>/dev/null 
#
exit
