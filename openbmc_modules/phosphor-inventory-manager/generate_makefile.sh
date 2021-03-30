#!/bin/sh

yaml_top=$1
toplevel_dirs=xyz
interfaces=

if [ -d $yaml_top/$toplevel_dirs ]; then
    cd $yaml_top
    interfaces=`find $toplevel_dirs -name "*.interface.yaml"`
fi

for i in ${interfaces};
do
    iface_path=`dirname $i`/`basename $i .interface.yaml`
    iface=`echo $iface_path | sed 's/\//./g'`
    cat <<MAKEFILE
${i%.interface.yaml}/server.cpp: \$(extra_yamldir)/${i} ${i%.interface.yaml}/server.hpp
	@mkdir -p \`dirname \$@\`
	\$(AM_V_GEN)\$(SDBUSPLUSPLUS) -r \$(extra_yamldir) interface server-cpp ${iface} > \$@

${i%.interface.yaml}/server.hpp: \$(extra_yamldir)/${i}
	@mkdir -p \`dirname \$@\`
	\$(AM_V_GEN)\$(SDBUSPLUSPLUS) -r \$(extra_yamldir) interface server-header ${iface} > \$@

MAKEFILE

done

echo "extra_ifaces_cpp_SOURCES = \\"
for i in ${interfaces};
do
    echo "	${i%.interface.yaml}/server.cpp \\"
done
echo

echo "extra_ifaces_hpp_SOURCES = \\"
for i in ${interfaces};
do
    echo "	${i%.interface.yaml}/server.hpp \\"
done
echo

echo "extra_ifaces.cpp: \$(extra_ifaces_cpp_SOURCES)"
if [ "$interfaces" ]; then
    echo "	\$(AM_V_GEN)cat \$^ > \$@"
else
    echo "	\$(AM_V_GEN)touch \$@"
fi

cat << MAKEFILE

.PHONY: clean-extra
clean-extra:
	for i in \$(extra_ifaces_cpp_SOURCES) \\
	         \$(extra_ifaces_hpp_SOURCES); \\
	do \\
	    test -e \$\$i && rm \$\$i ; \\
	    test -d \`dirname \$\$i\` && rmdir -p \`dirname \$\$i\` ; \\
	    true; \\
	done
MAKEFILE
