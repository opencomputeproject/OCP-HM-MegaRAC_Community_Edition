std::make_unique<AnyOf>(
${indent(1)}ConfigFans::get()[${f.fan}],
${indent(1)}std::vector<std::reference_wrapper<PresenceSensor>>{
% for s in f.sensors:
${indent(2)}*ConfigSensors::get()[${s}],
% endfor
${indent(1)}})\
