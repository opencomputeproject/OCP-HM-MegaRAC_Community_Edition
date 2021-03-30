std::make_unique<Journal<${c.datatype}, phosphor::logging::level::${c.severity}>>(
${indent(1)}"${c.message}",
${indent(1)}ConfigPropertyIndicies::get()[${c.instances}])\
