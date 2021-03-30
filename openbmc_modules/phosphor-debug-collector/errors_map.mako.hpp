## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include <map>
using EType = std::string;
using Error = std::string;
using ErrorList = std::vector<Error>;
using ErrorMap = std::map<EType, std::vector<Error>>;

const ErrorMap errorMap = {
% for key, errors in errDict.items():
    {"${key}", {
    % for error in errors:
        "${error}",
    % endfor
    }},
% endfor
};
