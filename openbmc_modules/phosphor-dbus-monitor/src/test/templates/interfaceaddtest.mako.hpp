const std::array<std::string, ${len(pathinstances)}> paths = {
% for p in paths:
    "${p.name}"s,
% endfor
};

const std::array<std::string, ${len(pathwatches)}> pathwatches = {{
% for w in pathwatches:
    paths[${w.pathinstances}],
% endfor
}};
