#!/bin/bash
devmem 0x1e6e2000 32 0x1688a8a8;devmem 0x1e6e207C 32 0x00400000;

flag=1
while [ $flag -eq 1 ]
do
count=$(ps | grep -w /usr/bin/kcsbridged | wc | awk '{print $1}')
if [ $count -gt 1 ]; then
	devmem 0x1e6e208c 32 0x00021004;devmem 0x1E780068 32 0x01000000;sleep 5;
	devmem 0x1e780020 32 0x00f45f37;sleep 1;devmem 0x1e780020 32 0x00f45f3f;
flag=0
fi
sleep 1
done

exit 0
