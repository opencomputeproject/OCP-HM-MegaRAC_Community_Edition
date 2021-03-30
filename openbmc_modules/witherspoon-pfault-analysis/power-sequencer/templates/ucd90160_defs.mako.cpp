/* This is a generated file. */

#include "ucd90160.hpp"

namespace witherspoon
{
namespace power
{

using namespace ucd90160;
using namespace std::string_literals;

const DeviceMap UCD90160::deviceMap
{
%for ucd_data in ucd90160s:
    {${ucd_data['index']},
     DeviceDefinition{
       "${ucd_data['path']}",

        RailNames{
        %for rail in ucd_data['RailNames']:
            "${rail}"s,
        %endfor
        },

        GPIConfigs{
        %for gpi_config in ucd_data['GPIConfigs']:
        <%
            poll = str(gpi_config['poll']).lower()
        %>\
            GPIConfig{${gpi_config['gpi']}, ${gpi_config['pinID']}, "${gpi_config['name']}"s, ${poll}, extraAnalysisType::${gpi_config['analysis']}},
        %endfor
        },

        GPIOAnalysis{
        %for gpio_analysis in ucd_data['GPIOAnalysis']:
             {extraAnalysisType::${gpio_analysis['type']},
              GPIOGroup{
                  "${gpio_analysis['path']}",
                  gpio::Value::${gpio_analysis['gpio_value']},
                  [](auto& ucd, const auto& callout) {
        ucd.${gpio_analysis['error_function']}(callout);
                  },
                  optionFlags::${gpio_analysis['option_flags']},
                  GPIODefinitions{
                  %for gpio_defs in gpio_analysis['GPIODefinitions']:
                      GPIODefinition{${gpio_defs['gpio']}, "${gpio_defs['callout']}"s},
                  %endfor
                  }
              }
             },
        %endfor
        }
     }
    },
%endfor
};

} // namespace power
} // namespace witherspoon
