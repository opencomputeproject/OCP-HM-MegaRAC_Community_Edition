auto storageCount = ${len(instances)};

const std::array<Index, ${len(instancegroups)}> indices = {{
% for g in instancegroups:
    {
    % for i in g.members:
        {Index::key_type{${i[0]}, ${i[2]}, ${i[3]}}, ${i[5]}},
    % endfor
    },
% endfor
}};

const std::array<std::tuple<std::string, size_t>, ${len(watches)}> watches = {{
% for w in watches:
    std::tuple<std::string, size_t>{"${w.datatype}", ${w.instances}},
% endfor
}};
