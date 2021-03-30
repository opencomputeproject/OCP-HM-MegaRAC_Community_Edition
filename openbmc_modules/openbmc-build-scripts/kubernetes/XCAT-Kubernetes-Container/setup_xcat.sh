#!/bin/bash

mkdir /root/xcat2
cd /root/xcat2
wget https://downloads.sourceforge.net/project/xcat/xcat-dep/2.x_Ubuntu/xcat-dep-ubuntu-snap20150611.tar.bz
wget https://github.com/xcat2/xcat-core/releases/download/2.10_release/xcat-core-2.10-ubuntu.tar.bz2
ls
tar jxvf xcat-dep-ubuntu-snap20150611.tar.bz
tar jxvf xcat-core-2.10-ubuntu.tar.bz2
./xcat-dep/mklocalrepo.sh
./xcat-core/mklocalrepo.sh
apt-get update && apt-get install -y --force-yes xcat
rm /root/xcat2/*.tar*
apt-get autoremove
