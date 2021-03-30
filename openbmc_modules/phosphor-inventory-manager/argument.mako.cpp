{
${indent(1)}${(',\n' + indent(1)).join([val.argument(loader, indent=indent +1) for val in arg.values])}
${indent(0)}}\
