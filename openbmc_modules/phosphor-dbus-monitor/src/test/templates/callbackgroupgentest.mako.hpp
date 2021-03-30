const std::array<std::vector<size_t>, ${len(callbackgroups)}> groups = {{
% for g in callbackgroups:
    {${', '.join([str(x) for x in g.members])}},
% endfor
}};
