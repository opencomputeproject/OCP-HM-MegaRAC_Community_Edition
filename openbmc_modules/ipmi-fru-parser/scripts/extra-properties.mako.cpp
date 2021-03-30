## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// WARNING: Generated source. Do not edit!

#include "types.hpp"

using namespace ipmi::vpd;

extern const std::map<Path, InterfaceMap> extras = {
% for path in dict.keys():
<%
    interfaces = dict[path]
%>\
    {"${path}",{
    % for interface,properties in interfaces.items():
        {"${interface}",{
        % for property,value in properties.items():
            {"${property}", ${value}},
        % endfor
        }},
    % endfor
    }},
% endfor
};
