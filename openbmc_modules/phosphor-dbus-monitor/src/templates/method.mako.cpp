makeMethod<SDBusPlus>(
${indent(1)}ConfigInterfaces::get()[${c.service}],
${indent(1)}ConfigPaths::get()[${c.path}],
${indent(1)}ConfigInterfaces::get()[${c.interface}],
${indent(1)}ConfigProperties::get()[${c.method}],
${indent(1)}${(',\n' + indent(1)).join([val.argument(loader, indent=indent +1) for val in c.args])})\
