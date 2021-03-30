${'::'.join(method.namespace + [method.name])}\
% if method.templates:
<${(',\n' + indent(1)).join([t.qualified() for t in method.templates])}>\
% endif
(\
% if method.args:

${indent(1)}${(',\n' + indent(1)).join([arg.argument(loader, indent=indent +1) for arg in method.args])})\
% endif
