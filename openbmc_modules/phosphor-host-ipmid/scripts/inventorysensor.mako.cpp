## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.

// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!

#include <ipmid/types.hpp>
using namespace ipmi::sensor;

extern const InvObjectIDMap invSensors = {
% for key in sensorDict.keys():
   % if key:
{"${key}",
    {
<%
       objectPath = sensorDict[key]
       sensorID = objectPath["sensorID"]
       sensorType = objectPath["sensorType"]
       eventReadingType = objectPath["eventReadingType"]
       offset = objectPath["offset"]
%>
        ${sensorID},${sensorType},${eventReadingType},${offset}
    }
},
   % endif
% endfor
};

