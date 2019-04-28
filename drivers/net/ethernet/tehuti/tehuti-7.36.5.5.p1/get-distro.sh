#!/bin/bash
# Check for GNU/Linux distributions
if [ -f /etc/redhat-release ]
then                                # RedHat
    if grep -iq 'red.*hat.*enterprise.*linux' \
        /etc/redhat-release
    then
        dist="RHEL"
    elif grep -iq 'CentOS' /etc/redhat-release
    then
        dist="CentOS"
    else
        dist="RH"
    fi
    release=$(sed -e 's#[^.0-9]##g' /etc/redhat-release)
elif [ -f /etc/fedora-release ]
then                                # Fedora
    dist="FED"
    release=$(sed -e 's#[^.0-9]##g' /etc/fedora-release)
elif [ -f /etc/SuSE-release ]
then                                # SuSE
    dist="SuSE"
    release=$(sed  -e '/SuSE.*/d' -e 's/VERSION[ =]*//' \
        /etc/SuSE-release)
elif [ -f /etc/mandrake-release ]
then                                # Mandrake
    dist="MDK"
    release=$(sed "s/.*release \([0-9.]*\).*/\1/g" \
        /etc/mandrake-release)
elif [ -f /etc/lsb-release ]
then                                # Ubuntu"
    dist="Ubuntu"
    release=$(awk -v FS="=" -- '/DISTRIB_RELEASE/ { print $2 }'\
                              /etc/lsb-release)
elif [ -f /etc/UnitedLinux-release ]
then                                # UnitedLinux
    dist="UL"
    release="?.?"                   # XXX Need to implement
elif [ -f /etc/debian_version ]
then                                # Debian
    dist="DEB"
    release="?.?"                   # XXX Need to implement
else                                # Unkonwn
    dist="unknown"
    release="?.?"
fi
echo "$dist:$release"

