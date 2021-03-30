std::make_unique<PolicyAccess<Tach, ConfigPolicy>>(
${indent(1)}${t.policy}, std::vector<std::string>{\
% for s in t.sensors:
"${s}",\
% endfor
})\
