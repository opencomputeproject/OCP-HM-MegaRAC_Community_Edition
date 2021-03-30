std::make_unique<MedianCondition<${c.datatype}>>(
${indent(1)}ConfigPropertyIndicies::get()[${c.instances}],
${indent(1)}[](const auto& item){return item ${c.op} ${c.bound.argument(loader, indent=indent +1)};},
${indent(1)}${c.oneshot.argument(loader, indent=indent +1)})\
