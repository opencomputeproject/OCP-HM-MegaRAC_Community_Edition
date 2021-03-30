const std::array<std::vector<size_t>, ${len(callbackgroups)}> groups = {{
% for g in callbackgroups:
    {${', '.join([str(x) for x in g.members])}},
% endfor
}};

<% graphs = [ x for x in callbacks if hasattr(x, 'graph')] %>\
const std::array<size_t, ${len(graphs)}> callbacks = {
% for g in graphs:
    ${g.graph},
% endfor
};
