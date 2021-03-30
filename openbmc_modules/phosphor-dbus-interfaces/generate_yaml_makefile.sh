#!/bin/sh

cd $1

toplevel_dirs=$2
all_yaml=`find $toplevel_dirs -name "*.yaml"`

echo "nobase_yaml_DATA = \\"
for i in ${all_yaml};
do
    echo "	${i} \\"
done
echo
