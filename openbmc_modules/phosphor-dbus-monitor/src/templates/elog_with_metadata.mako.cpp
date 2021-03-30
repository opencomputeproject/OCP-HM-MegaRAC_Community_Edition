std::make_unique<ElogWithMetadataCapture<
${indent(1)}sdbusplus::${c.error},
${indent(1)}phosphor::logging::${c.metadata},
${indent(1)}${c.datatype}>>(
${indent(1)}ConfigPropertyIndicies::get()[${c.instances}])\
