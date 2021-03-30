// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include <iostream>
#include "fruread.hpp"

extern const FruMap frus = {
% for key in fruDict.keys():
   {${key},{
<%
    instanceList = fruDict[key]
%>
    % for instancePath,instanceInfo in instanceList.items():
<%
        entityID = instanceInfo["entityID"]
        entityInstance = instanceInfo["entityInstance"]
        interfaces = instanceInfo["interfaces"]
%>
         {${entityID}, ${entityInstance}, "${instancePath}",{
         % for interface,properties in interfaces.items():
             {"${interface}",{
            % if properties:
                % for dbus_property,property_value in properties.items():
                    {"${dbus_property}",{
                        "${property_value.get("IPMIFruSection", "")}",
                        "${property_value.get("IPMIFruProperty", "")}",\
<%
    delimiter = property_value.get("IPMIFruValueDelimiter")
    if not delimiter:
        delimiter = ""
    else:
        delimiter = '\\' + hex(delimiter)[1:]
%>
                     "${delimiter}"
                 }},
                % endfor
            %endif
             }},
         % endfor
        }},
    % endfor
   }},
% endfor
};
