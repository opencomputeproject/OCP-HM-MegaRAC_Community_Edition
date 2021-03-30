makeElog<sdbusplus::${c.error}>(
${indent(1)}${(',\n' + indent(1)).join(["phosphor::logging::" + meta.name + '(' +
meta.argument(loader, indent=indent +1) + ')' for meta in c.metadata]) })\
