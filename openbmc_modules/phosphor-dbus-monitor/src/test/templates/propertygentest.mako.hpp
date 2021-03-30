const std::array<std::string, ${len(meta)}> meta = {
% for m in meta:
    "${m.name}"s,
% endfor
};

const std::array<std::string, ${len(interfaces)}> interfaces = {
% for i in interfaces:
    "${i.name}"s,
% endfor
};

const std::array<std::string, ${len(propertynames)}> properties = {
% for p in propertynames:
    "${p.name}"s,
% endfor
};

const std::array<GroupOfProperties, ${len(propertygroups)}> groups = {{
% for g in propertygroups:
    // ${g.name}
    {
        % for p in g.members:
        ::Property{ interfaces[${p[0]}], properties[${p[1]}], meta[${p[2]}] },
        % endfor
    },
% endfor
}};

const std::array<std::string, ${len(propertygroups)}> types = {
% for g in propertygroups:
    "${g.datatype}"s,
% endfor
};
