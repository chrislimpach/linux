#!/bin/bash
CLEAN=NO

PHY_BINFILE=MV88X3120_phy_firmware.hdr
PHY_HEADER_FILE=MV88X3120_phy_firmware.h
#echo $0 $*

while [ "$1" != "" ]; do
    case $1 in
	astyle)
	    astyle -A1 -s -S -Y -c < OEM_DRVNAME_.c > XOEM_DRVNAME_.c
	    astyle -A1 -s -S -Y -c < OEM_DRVNAME_.h > XOEM_DRVNAME_.h
	    exit
	    ;;
	compile)
	    make
	    exit
	    ;;
	phy)
	    xxd -i -c2 ${PHY_BINFILE} /tmp/${PHY_HEADER_FILE}
	    sed   '
1s/char/static u16/
s/, 0x//g' </tmp/${PHY_HEADER_FILE}  >${PHY_HEADER_FILE}
	    rm /tmp/${PHY_HEADER_FILE}
	    exit
	    ;;

	clean)
	    CLEAN=YES
	    ;;

        * )
	    echo "Unknown option $1, done nothing !"
	    exit
	    ;;
    esac
    shift
done

if [ "${CLEAN}" == "YES" ]; then
    if [ "${TARGET}" == "" ]; then
	TARGET=clean
    else
	TARGET=clean_${TARGET}
    fi
fi

make ${TARGET}

exit
