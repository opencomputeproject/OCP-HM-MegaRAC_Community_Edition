<%include file="defs.mako"/>\
<%namespace file="defs.mako" import="*"/>\
<%!
def indent(str, depth):
    return ''.join(4*' '*depth+line for line in str.splitlines(True))
%>\
/* This is a generated file. */
#include "manager.hpp"
#include "functor.hpp"
#include "actions.hpp"
#include "handlers.hpp"
#include "preconditions.hpp"
#include "matches.hpp"
#include "triggers.hpp"

using namespace phosphor::fan::control;

const unsigned int Manager::_powerOnDelay{${mgr_data['power_on_delay']}};

const std::vector<ZoneGroup> Manager::_zoneLayouts
{
%for zone_group in zones:
    ZoneGroup{
        std::vector<Condition>{
        %for condition in zone_group['conditions']:
            Condition{
                "${condition['type']}",
                std::vector<ConditionProperty>{
                %for property in condition['properties']:
                    ConditionProperty{
                        "${property['property']}",
                        "${property['interface']}",
                        "${property['path']}",
                        static_cast<${property['type']}>(${property['value']}),
                    },
                    %endfor
                },
            },
            %endfor
        },
        std::vector<ZoneDefinition>{
        %for zone in zone_group['zones']:
            ZoneDefinition{
                ${zone['num']},
                ${zone['full_speed']},
                ${zone['default_floor']},
                ${zone['increase_delay']},
                ${zone['decrease_interval']},
                std::vector<ZoneHandler>{
                    %if ('ifaces' in zone) and \
                        (zone['ifaces'] is not None):
                        %for i in zone['ifaces']:
                            %if ('props' in i) and \
                                (i['props'] is not None):
                                %for p in i['props']:
                    ZoneHandler{
                        make_zoneHandler(handler::setZoneProperty(
                            "${i['name']}",
                            "${p['name']}",
                            &Zone::${p['func']},
                            static_cast<${p['type']}>(
                                %if "vector" in p['type'] or "map" in p['type']:
                                ${p['type']}{
                                %endif
                                %for j, v in enumerate(p['values']):
                                %if (j+1) != len(p['values']):
                                    ${v},
                                %else:
                                    ${v}
                                %endif
                                %endfor
                                %if "vector" in p['type'] or "map" in p['type']:
                                }
                                %endif
                            ),
                            ${p['persist']}
                        ))
                    },
                                %endfor
                            %endif
                        %endfor
                    %endif
                },
                std::vector<FanDefinition>{
                %for fan in zone['fans']:
                    FanDefinition{
                        "${fan['name']}",
                        std::vector<std::string>{
                        %for sensor in fan['sensors']:
                            "${sensor}",
                        %endfor
                        },
                        "${fan['target_interface']}"
                    },
                %endfor
                },
                std::vector<SetSpeedEvent>{
                %for event in zone['events']:
                    %if ('pc' in event) and \
                        (event['pc'] is not None):
                    SetSpeedEvent{
                        "${event['pc']['pcname']}",
                        Group
                        {
                        %for group in event['pc']['pcgrps']:
                        %for member in group['members']:
                            {"${member['object']}",
                            "${member['interface']}",
                            "${member['property']}"},
                        %endfor
                        %endfor
                        },
                        ActionData{
                        {Group{},
                        std::vector<Action>{
                        %for i, a in enumerate(event['pc']['pcact']):
                        make_action(
                            precondition::${a['name']}(
                        %for p in a['params']:
                        ${p['type']}${p['open']}
                        %for j, v in enumerate(p['values']):
                        %if (j+1) != len(p['values']):
                            ${v['value']},
                        %else:
                            ${v['value']}
                        %endif
                        %endfor
                        ${p['close']},
                        %endfor
                        %endfor
                    std::vector<SetSpeedEvent>{
                    %for pcevt in event['pc']['pcevts']:
                    SetSpeedEvent{
                        "${pcevt['name']}",\
                    ${indent(genSSE(event=pcevt), 6)}\
                    },
                    %endfor
                    %else:
                    SetSpeedEvent{
                        "${event['name']}",\
                    ${indent(genSSE(event=event), 6)}
                    %endif
                    %if ('pc' in event) and (event['pc'] is not None):
                    }
                        )),
                        }},
                        },
                        std::vector<Trigger>{
                            %if ('timer' in event['pc']['triggers']) and \
                                (event['pc']['triggers']['timer'] is not None):
                            make_trigger(trigger::timer(TimerConf{
                            ${event['pc']['triggers']['pctime']['interval']},
                            ${event['pc']['triggers']['pctime']['type']}
                            })),
                            %endif
                            %if ('pcsigs' in event['pc']['triggers']) and \
                                (event['pc']['triggers']['pcsigs'] is not None):
                            %for s in event['pc']['triggers']['pcsigs']:
                            make_trigger(trigger::signal(
                                %if ('match' in s) and \
                                    (s['match'] is not None):
                                match::${s['match']}(
                                %for i, mp in enumerate(s['mparams']['params']):
                                %if (i+1) != len(s['mparams']['params']):
                                ${indent(s['mparams'][mp], 1)},
                                %else:
                                ${indent(s['mparams'][mp], 1)}
                                %endif
                                %endfor
                                ),
                                %else:
                                "",
                                %endif
                                make_handler<SignalHandler>(\
                                ${indent(genSignal(sig=s), 9)}\
                                )
                            )),
                            %endfor
                            %endif
                            %if ('init' in event['pc']['triggers']):
                            %for i in event['pc']['triggers']['init']:
                            make_trigger(trigger::init(
                                %if ('method' in i):
                                make_handler<MethodHandler>(\
                                ${indent(genMethod(meth=i), 3)}\
                                )
                                %endif
                            )),
                            %endfor
                            %endif
                        },
                    %endif
                    },
                %endfor
                }
            },
        %endfor
        }
    },
%endfor
};
