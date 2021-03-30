% for i, s in enumerate(signature.sig.items()):
    % if i == 0:
${'"{0}=\'{1}\',"'.format(*s)}
    % elif i + 1 == len(signature.sig):
${indent(1)}${'"{0}=\'{1}\'"'.format(*s)}\
    % else:
${indent(1)}${'"{0}=\'{1}\',"'.format(*s)}
    % endif
% endfor
