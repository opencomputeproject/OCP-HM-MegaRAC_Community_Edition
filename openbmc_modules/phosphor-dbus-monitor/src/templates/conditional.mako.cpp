% if c.defer:
std::make_unique<DeferrableCallback<ConfigPropertyCallbacks>>(
${indent(1)}ConfigPropertyCallbackGroups::get()[${c.graph}],
${indent(1)}*ConfigConditions::get()[${c.condition}],
${indent(1)}${c.defer})\
% else:
std::make_unique<ConditionalCallback<ConfigPropertyCallbacks>>(
${indent(1)}ConfigPropertyCallbackGroups::get()[${c.graph}],
${indent(1)}*ConfigConditions::get()[${c.condition}])\
% endif\
