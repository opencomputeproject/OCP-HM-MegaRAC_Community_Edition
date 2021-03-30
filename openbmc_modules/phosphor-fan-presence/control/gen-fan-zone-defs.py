#!/usr/bin/env python3

"""
This script reads in fan definition and zone definition YAML
files and generates a set of structures for use by the fan control code.
"""

import os
import sys
import yaml
from argparse import ArgumentParser
from mako.template import Template
from mako.lookup import TemplateLookup


def parse_cpp_type(typeName):
    """
    Take a list of dbus types from YAML and convert it to a recursive cpp
    formed data structure. Each entry of the original list gets converted
    into a tuple consisting of the type name and a list with the params
    for this type,
        e.g.
            ['dict', ['string', 'dict', ['string', 'int64']]]
        is converted to
            [('dict', [('string', []), ('dict', [('string', []),
             ('int64', [])]]]
    """

    if not typeName:
        return None

    # Type names are _almost_ valid YAML. Insert a , before each [
    # and then wrap it in a [ ] and it becomes valid YAML (assuming
    # the user gave us a valid typename).
    typeArray = yaml.safe_load("[" + ",[".join(typeName.split("[")) + "]")
    typeTuple = preprocess_yaml_type_array(typeArray).pop(0)
    return get_cpp_type(typeTuple)


def preprocess_yaml_type_array(typeArray):
    """
    Flattens an array type into a tuple list that can be used to get the
    supported cpp type from each element.
    """

    result = []

    for i in range(len(typeArray)):
        # Ignore lists because we merge them with the previous element
        if type(typeArray[i]) is list:
            continue

        # If there is a next element and it is a list, merge it with the
        # current element.
        if i < len(typeArray)-1 and type(typeArray[i+1]) is list:
            result.append(
                (typeArray[i],
                 preprocess_yaml_type_array(typeArray[i+1])))
        else:
            result.append((typeArray[i], []))

    return result


def get_cpp_type(typeTuple):
    """
    Take a list of dbus types and perform validity checking, such as:
        [ variant [ dict [ int32, int32 ], double ] ]
    This function then converts the type-list into a C++ type string.
    """

    propertyMap = {
        'byte': {'cppName': 'uint8_t', 'params': 0},
        'boolean': {'cppName': 'bool', 'params': 0},
        'int16': {'cppName': 'int16_t', 'params': 0},
        'uint16': {'cppName': 'uint16_t', 'params': 0},
        'int32': {'cppName': 'int32_t', 'params': 0},
        'uint32': {'cppName': 'uint32_t', 'params': 0},
        'int64': {'cppName': 'int64_t', 'params': 0},
        'uint64': {'cppName': 'uint64_t', 'params': 0},
        'double': {'cppName': 'double', 'params': 0},
        'string': {'cppName': 'std::string', 'params': 0},
        'array': {'cppName': 'std::vector', 'params': 1},
        'dict': {'cppName': 'std::map', 'params': 2}}

    if len(typeTuple) != 2:
        raise RuntimeError("Invalid typeTuple %s" % typeTuple)

    first = typeTuple[0]
    entry = propertyMap[first]

    result = entry['cppName']

    # Handle 0-entry parameter lists.
    if (entry['params'] == 0):
        if (len(typeTuple[1]) != 0):
            raise RuntimeError("Invalid typeTuple %s" % typeTuple)
        else:
            return result

    # Get the parameter list
    rest = typeTuple[1]

    # Confirm parameter count matches.
    if (entry['params'] != -1) and (entry['params'] != len(rest)):
        raise RuntimeError("Invalid entry count for %s : %s" %
                           (first, rest))

    # Parse each parameter entry, if appropriate, and create C++ template
    # syntax.
    result += '<'
    if entry.get('noparse'):
        # Do not parse the parameter list, just use the first element
        # of each tuple and ignore possible parameters
        result += ", ".join([e[0] for e in rest])
    else:
        result += ", ".join([get_cpp_type(e) for e in rest])
    result += '>'

    return result


def convertToMap(listOfDict):
    """
    Converts a list of dictionary entries to a std::map initialization list.
    """
    listOfDict = listOfDict.replace('\'', '\"')
    listOfDict = listOfDict.replace('[', '{')
    listOfDict = listOfDict.replace(']', '}')
    listOfDict = listOfDict.replace(':', ',')
    return listOfDict


def genEvent(event):
    """
    Generates the source code of an event and returns it as a string
    """
    e = "SetSpeedEvent{\n"
    e += "\"" + event['name'] + "\",\n"
    e += "Group{\n"
    for group in event['groups']:
        for member in group['members']:
            e += "{\"" + member['object'] + "\",\n"
            e += "\"" + member['interface'] + "\",\n"
            e += "\"" + member['property'] + "\"},\n"
    e += "},\n"

    e += "ActionData{\n"
    for d in event['action']:
        e += "{Group{\n"
        for g in d['groups']:
            for m in g['members']:
                e += "{\"" + m['object'] + "\",\n"
                e += "\"" + m['interface'] + "\",\n"
                e += "\"" + m['property'] + "\"},\n"
        e += "},\n"
        e += "std::vector<Action>{\n"
        for a in d['actions']:
            if len(a['parameters']) != 0:
                e += "make_action(action::" + a['name'] + "(\n"
            else:
                e += "make_action(action::" + a['name'] + "\n"
            for i, p in enumerate(a['parameters']):
                if (i+1) != len(a['parameters']):
                    e += p + ",\n"
                else:
                    e += p + "\n"
            if len(a['parameters']) != 0:
                e += ")),\n"
            else:
                e += "),\n"
        e += "}},\n"
    e += "},\n"

    e += "std::vector<Trigger>{\n"
    if ('timer' in event['triggers']) and \
       (event['triggers']['timer'] is not None):
       e += "\tmake_trigger(trigger::timer(TimerConf{\n"
       e += "\t" + event['triggers']['timer']['interval'] + ",\n"
       e += "\t" + event['triggers']['timer']['type'] + "\n"
       e += "\t})),\n"

    if ('signals' in event['triggers']) and \
       (event['triggers']['signals'] is not None):
        for s in event['triggers']['signals']:
            e += "\tmake_trigger(trigger::signal(\n"
            e += "match::" + s['match'] + "(\n"
            for i, mp in enumerate(s['mparams']['params']):
                if (i+1) != len(s['mparams']['params']):
                    e += "\t\t\t" + s['mparams'][mp] + ",\n"
                else:
                    e += "\t\t\t" + s['mparams'][mp] + "\n"
            e += "\t\t),\n"
            e += "\t\tmake_handler<SignalHandler>(\n"
            if ('type' in s['sparams']) and (s['sparams']['type'] is not None):
                e += s['signal'] + "<" + s['sparams']['type'] + ">(\n"
            else:
                e += s['signal'] + "(\n"
            for sp in s['sparams']['params']:
                e += s['sparams'][sp] + ",\n"
            if ('type' in s['hparams']) and (s['hparams']['type'] is not None):
                e += ("handler::" + s['handler'] +
                      "<" + s['hparams']['type'] + ">(\n")
            else:
                e += "handler::" + s['handler'] + "(\n)"
            for i, hp in enumerate(s['hparams']['params']):
                if (i+1) != len(s['hparams']['params']):
                    e += s['hparams'][hp] + ",\n"
                else:
                    e += s['hparams'][hp] + "\n"
            e += "))\n"
            e += "\t\t)\n"
            e += "\t)),\n"

    if ('init' in event['triggers']):
        for i in event['triggers']['init']:
            e += "\tmake_trigger(trigger::init(\n"
            if ('method' in i):
                e += "\t\tmake_handler<MethodHandler>(\n"
                if ('type' in i['mparams']) and \
                    (i['mparams']['type'] is not None):
                    e += i['method'] + "<" + i['mparams']['type'] + ">(\n"
                else:
                    e += i['method'] + "(\n"
                for ip in i['mparams']['params']:
                    e += i['mparams'][ip] + ",\n"
                if ('type' in i['hparams']) and \
                    (i['hparams']['type'] is not None):
                    e += ("handler::" + i['handler'] +
                        "<" + i['hparams']['type'] + ">(\n")
                else:
                    e += "handler::" + i['handler'] + "(\n)"
                for i, hp in enumerate(i['hparams']['params']):
                    if (i+1) != len(i['hparams']['params']):
                        e += i['hparams'][hp] + ",\n"
                    else:
                        e += i['hparams'][hp] + "\n"
                e += "))\n"
                e += "\t\t)\n"
            e += "\t)),\n"

    e += "},\n"

    e += "}"

    return e


def getGroups(zNum, zCond, edata, events):
    """
    Extract and construct the groups for the given event.
    """
    groups = []
    if ('groups' in edata) and (edata['groups'] is not None):
        for eGroups in edata['groups']:
            if ('zone_conditions' in eGroups) and \
               (eGroups['zone_conditions'] is not None):
                # Zone conditions are optional in the events yaml but skip
                # if this event's condition is not in this zone's conditions
                if all('name' in z and z['name'] is not None and
                       not any(c['name'] == z['name'] for c in zCond)
                       for z in eGroups['zone_conditions']):
                    continue

                # Zone numbers are optional in the events yaml but skip if this
                # zone's zone number is not in the event's zone numbers
                if all('zones' in z and z['zones'] is not None and
                       zNum not in z['zones']
                       for z in eGroups['zone_conditions']):
                    continue
            eGroup = next(g for g in events['groups']
                          if g['name'] == eGroups['name'])

            group = {}
            members = []
            group['name'] = eGroup['name']
            for m in eGroup['members']:
                member = {}
                member['path'] = eGroup['type']
                member['object'] = (eGroup['type'] + m)
                member['interface'] = eGroups['interface']
                member['property'] = eGroups['property']['name']
                member['type'] = eGroups['property']['type']
                # Use defined service to note member on zone object
                if ('service' in eGroup) and \
                   (eGroup['service'] is not None):
                    member['service'] = eGroup['service']
                # Add expected group member's property value if given
                if ('value' in eGroups['property']) and \
                   (eGroups['property']['value'] is not None):
                        if isinstance(eGroups['property']['value'], str) or \
                                "string" in str(member['type']).lower():
                            member['value'] = (
                                "\"" + eGroups['property']['value'] + "\"")
                        else:
                            member['value'] = eGroups['property']['value']
                members.append(member)
            group['members'] = members
            groups.append(group)
    return groups


def getParameters(member, groups, section, events):
    """
    Extracts and constructs a section's parameters
    """
    params = {}
    if ('parameters' in section) and \
       (section['parameters'] is not None):
        plist = []
        for sp in section['parameters']:
            p = str(sp)
            if (p != 'type'):
                plist.append(p)
                if (p != 'group'):
                    params[p] = "\"" + member[p] + "\""
                else:
                    params[p] = "Group\n{\n"
                    for g in groups:
                        for m in g['members']:
                            params[p] += (
                                "{\"" + str(m['object']) + "\",\n" +
                                "\"" + str(m['interface']) + "\",\n" +
                                "\"" + str(m['property']) + "\"},\n")
                    params[p] += "}"
            else:
                params[p] = member[p]
        params['params'] = plist
    else:
        params['params'] = []
    return params


def getInit(eGrps, eTrig, events):
    """
    Extracts and constructs an init trigger for the event's groups
    which are required to be of the same type.
    """
    method = {}
    methods = []
    if (len(eGrps) > 0):
        # Use the first group member for retrieving the type
        member = eGrps[0]['members'][0]
        if ('method' in eTrig) and \
           (eTrig['method'] is not None):
            # Add method parameters
            eMethod = next(m for m in events['methods']
                           if m['name'] == eTrig['method'])
            method['method'] = eMethod['name']
            method['mparams'] = getParameters(
                member, eGrps, eMethod, events)

            # Add handler parameters
            eHandler = next(h for h in events['handlers']
                            if h['name'] == eTrig['handler'])
            method['handler'] = eHandler['name']
            method['hparams'] = getParameters(
                member, eGrps, eHandler, events)

    methods.append(method)

    return methods


def getSignal(eGrps, eTrig, events):
    """
    Extracts and constructs for each group member a signal
    subscription of each match listed in the trigger.
    """
    signals = []
    for group in eGrps:
        for member in group['members']:
            signal = {}
            # Add signal parameters
            eSignal = next(s for s in events['signals']
                           if s['name'] == eTrig['signal'])
            signal['signal'] = eSignal['name']
            signal['sparams'] = getParameters(member, eGrps, eSignal, events)

            # If service not given, subscribe to signal match
            if ('service' not in member):
                # Add signal match parameters
                eMatch = next(m for m in events['matches']
                              if m['name'] == eSignal['match'])
                signal['match'] = eMatch['name']
                signal['mparams'] = getParameters(member, eGrps, eMatch, events)

            # Add handler parameters
            eHandler = next(h for h in events['handlers']
                            if h['name'] == eTrig['handler'])
            signal['handler'] = eHandler['name']
            signal['hparams'] = getParameters(member, eGrps, eHandler, events)

            signals.append(signal)

    return signals


def getTimer(eTrig):
    """
    Extracts and constructs the required parameters for an
    event timer.
    """
    timer = {}
    timer['interval'] = (
        "static_cast<std::chrono::microseconds>" +
        "(" + str(eTrig['interval']) + ")")
    timer['type'] = "TimerType::" + str(eTrig['type'])
    return timer


def getActions(zNum, zCond, edata, actions, events):
    """
    Extracts and constructs the make_action function call for
    all the actions within the given event.
    """
    action = []
    for eActions in actions['actions']:
        actions = {}
        eAction = next(a for a in events['actions']
                       if a['name'] == eActions['name'])
        actions['name'] = eAction['name']
        actions['groups'] = getGroups(zNum, zCond, eActions, events)
        params = []
        if ('parameters' in eAction) and \
           (eAction['parameters'] is not None):
            for p in eAction['parameters']:
                param = "static_cast<"
                if type(eActions[p]) is not dict:
                    if p == 'actions':
                        param = "std::vector<Action>{"
                        pActs = getActions(zNum,
                                           zCond,
                                           edata,
                                           eActions,
                                           events)
                        for a in pActs:
                            if (len(a['parameters']) != 0):
                                param += (
                                    "make_action(action::" +
                                    a['name'] +
                                    "(\n")
                                for i, ap in enumerate(a['parameters']):
                                    if (i+1) != len(a['parameters']):
                                        param += (ap + ",")
                                    else:
                                        param += (ap + ")")
                            else:
                                param += ("make_action(action::" + a['name'])
                            param += "),"
                        param += "}"
                    elif p == 'defevents' or p == 'altevents' or p == 'events':
                        param = "std::vector<SetSpeedEvent>{\n"
                        for i, e in enumerate(eActions[p]):
                            aEvent = getEvent(zNum, zCond, e, events)
                            if not aEvent:
                                continue
                            if (i+1) != len(eActions[p]):
                                param += genEvent(aEvent) + ",\n"
                            else:
                                param += genEvent(aEvent) + "\n"
                        param += "\t}"
                    elif p == 'property':
                        if isinstance(eActions[p], str) or \
                           "string" in str(eActions[p]['type']).lower():
                            param += (
                                str(eActions[p]['type']).lower() +
                                ">(\"" + str(eActions[p]) + "\")")
                        else:
                            param += (
                                str(eActions[p]['type']).lower() +
                                ">(" + str(eActions[p]['value']).lower() + ")")
                    else:
                        # Default type to 'size_t' when not given
                        param += ("size_t>(" + str(eActions[p]).lower() + ")")
                else:
                    if p == 'timer':
                        t = getTimer(eActions[p])
                        param = (
                            "TimerConf{" + t['interval'] + "," +
                            t['type'] + "}")
                    else:
                        param += (str(eActions[p]['type']).lower() + ">(")
                        if p != 'map':
                            if isinstance(eActions[p]['value'], str) or \
                               "string" in str(eActions[p]['type']).lower():
                                param += \
                                    "\"" + str(eActions[p]['value']) + "\")"
                            else:
                                param += \
                                    str(eActions[p]['value']).lower() + ")"
                        else:
                            param += (
                                str(eActions[p]['type']).lower() +
                                convertToMap(str(eActions[p]['value'])) + ")")
                params.append(param)
        actions['parameters'] = params
        action.append(actions)
    return action


def getEvent(zone_num, zone_conditions, e, events_data):
    """
    Parses the sections of an event and populates the properties
    that construct an event within the generated source.
    """
    event = {}

    # Add set speed event name
    event['name'] = e['name']

    # Add set speed event groups
    event['groups'] = getGroups(zone_num, zone_conditions, e, events_data)

    # Add optional set speed actions and function parameters
    event['action'] = []
    if ('actions' in e) and \
       (e['actions'] is not None):
        # List of dicts containing the list of groups and list of actions
        sseActions = []
        eActions = getActions(zone_num, zone_conditions, e, e, events_data)
        for eAction in eActions:
            # Skip events that have no groups defined for the event or actions
            if not event['groups'] and not eAction['groups']:
                continue
            # Find group in sseActions
            grpExists = False
            for sseDict in sseActions:
                if eAction['groups'] == sseDict['groups']:
                    # Extend 'actions' list
                    del eAction['groups']
                    sseDict['actions'].append(eAction)
                    grpExists = True
                    break
            if not grpExists:
                grps = eAction['groups']
                del eAction['groups']
                actList = []
                actList.append(eAction)
                sseActions.append({'groups': grps, 'actions': actList})
        event['action'] = sseActions

    # Add event triggers
    event['triggers'] = {}
    for trig in e['triggers']:
        triggers = []
        if (trig['name'] == "timer"):
            event['triggers']['timer'] = getTimer(trig)
        elif (trig['name'] == "signal"):
            if ('signals' not in event['triggers']):
                event['triggers']['signals'] = []
            triggers = getSignal(event['groups'], trig, events_data)
            event['triggers']['signals'].extend(triggers)
        elif (trig['name'] == "init"):
            triggers = getInit(event['groups'], trig, events_data)
            event['triggers']['init'] = triggers

    return event


def addPrecondition(zNum, zCond, event, events_data):
    """
    Parses the precondition section of an event and populates the necessary
    structures to generate a precondition for a set speed event.
    """
    precond = {}

    # Add set speed event precondition name
    precond['pcname'] = event['name']

    # Add set speed event precondition group
    precond['pcgrps'] = getGroups(zNum,
                                  zCond,
                                  event['precondition'],
                                  events_data)

    # Add set speed event precondition actions
    pc = []
    pcs = {}
    pcs['name'] = event['precondition']['name']
    epc = next(p for p in events_data['preconditions']
               if p['name'] == event['precondition']['name'])
    params = []
    for p in epc['parameters'] or []:
        param = {}
        if p == 'groups':
            param['type'] = "std::vector<PrecondGroup>"
            param['open'] = "{"
            param['close'] = "}"
            values = []
            for group in precond['pcgrps']:
                for pcgrp in group['members']:
                    value = {}
                    value['value'] = (
                        "PrecondGroup{\"" +
                        str(pcgrp['object']) + "\",\"" +
                        str(pcgrp['interface']) + "\",\"" +
                        str(pcgrp['property']) + "\"," +
                        "static_cast<" +
                        str(pcgrp['type']).lower() + ">")
                    if isinstance(pcgrp['value'], str) or \
                       "string" in str(pcgrp['type']).lower():
                        value['value'] += ("(" + str(pcgrp['value']) + ")}")
                    else:
                        value['value'] += \
                            ("(" + str(pcgrp['value']).lower() + ")}")
                    values.append(value)
            param['values'] = values
        params.append(param)
    pcs['params'] = params
    pc.append(pcs)
    precond['pcact'] = pc

    pcevents = []
    for pce in event['precondition']['events']:
        pcevent = getEvent(zNum, zCond, pce, events_data)
        if not pcevent:
            continue
        pcevents.append(pcevent)
    precond['pcevts'] = pcevents

    # Add precondition event triggers
    precond['triggers'] = {}
    for trig in event['precondition']['triggers']:
        triggers = []
        if (trig['name'] == "timer"):
            precond['triggers']['pctime'] = getTimer(trig)
        elif (trig['name'] == "signal"):
            if ('pcsigs' not in precond['triggers']):
                precond['triggers']['pcsigs'] = []
            triggers = getSignal(precond['pcgrps'], trig, events_data)
            precond['triggers']['pcsigs'].extend(triggers)
        elif (trig['name'] == "init"):
            triggers = getInit(precond['pcgrps'], trig, events_data)
            precond['triggers']['init'] = triggers

    return precond


def getEventsInZone(zone_num, zone_conditions, events_data):
    """
    Constructs the event entries defined for each zone using the events yaml
    provided.
    """
    events = []

    if 'events' in events_data:
        for e in events_data['events']:
            event = {}

            # Add precondition if given
            if ('precondition' in e) and \
               (e['precondition'] is not None):
                event['pc'] = addPrecondition(zone_num,
                                              zone_conditions,
                                              e,
                                              events_data)
            else:
                event = getEvent(zone_num, zone_conditions, e, events_data)
                # Remove empty events and events that have
                # no groups defined for the event or any of the actions
                if not event or \
                    (not event['groups'] and
                        all(not a['groups'] for a in event['action'])):
                    continue
            events.append(event)

    return events


def getFansInZone(zone_num, profiles, fan_data):
    """
    Parses the fan definition YAML files to find the fans
    that match both the zone passed in and one of the
    cooling profiles.
    """

    fans = []

    for f in fan_data['fans']:

        if zone_num != f['cooling_zone']:
            continue

        # 'cooling_profile' is optional (use 'all' instead)
        if f.get('cooling_profile') is None:
            profile = "all"
        else:
            profile = f['cooling_profile']

        if profile not in profiles:
            continue

        fan = {}
        fan['name'] = f['inventory']
        fan['sensors'] = f['sensors']
        fan['target_interface'] = f.get(
            'target_interface',
            'xyz.openbmc_project.Control.FanSpeed')
        fans.append(fan)

    return fans


def getIfacesInZone(zone_ifaces):
    """
    Parse given interfaces for a zone for associating a zone with an interface
    and set any properties listed to defined values upon fan control starting
    on the zone.
    """

    ifaces = []
    for i in zone_ifaces:
        iface = {}
        # Interface name not needed yet for fan zones but
        # may be necessary as more interfaces are extended by the zones
        iface['name'] = i['name']

        if ('properties' in i) and \
                (i['properties'] is not None):
            props = []
            for p in i['properties']:
                prop = {}
                prop['name'] = p['name']
                prop['func'] = str(p['name']).lower()
                prop['type'] = parse_cpp_type(p['type'])
                if ('persist' in p):
                    persist = p['persist']
                    if (persist is not None):
                        if (isinstance(persist, bool)):
                            prop['persist'] = 'true' if persist else 'false'
                else:
                    prop['persist'] = 'false'
                vals = []
                for v in p['values']:
                    val = v['value']
                    if (val is not None):
                        if (isinstance(val, bool)):
                            # Convert True/False to 'true'/'false'
                            val = 'true' if val else 'false'
                        elif (isinstance(val, str)):
                            # Wrap strings with double-quotes
                            val = "\"" + val + "\""
                        vals.append(val)
                prop['values'] = vals
                props.append(prop)
            iface['props'] = props
        ifaces.append(iface)

    return ifaces


def getConditionInZoneConditions(zone_condition, zone_conditions_data):
    """
    Parses the zone conditions definition YAML files to find the condition
    that match both the zone condition passed in.
    """

    condition = {}

    for c in zone_conditions_data['conditions']:

        if zone_condition != c['name']:
            continue
        condition['type'] = c['type']
        properties = []
        for p in c['properties']:
            property = {}
            property['property'] = p['property']
            property['interface'] = p['interface']
            property['path'] = p['path']
            property['type'] = p['type'].lower()
            property['value'] = str(p['value']).lower()
            properties.append(property)
        condition['properties'] = properties

        return condition


def buildZoneData(zone_data, fan_data, events_data, zone_conditions_data):
    """
    Combines the zone definition YAML and fan
    definition YAML to create a data structure defining
    the fan cooling zones.
    """

    zone_groups = []

    # Allow zone_conditions to not be in yaml (since its optional)
    if not isinstance(zone_data, list) and zone_data != {}:
        zone_data = [zone_data]
    for group in zone_data:
        conditions = []
        # zone conditions are optional
        if 'zone_conditions' in group and group['zone_conditions'] is not None:
            for c in group['zone_conditions']:

                if not zone_conditions_data:
                    sys.exit("No zone_conditions YAML file but " +
                             "zone_conditions used in zone YAML")

                condition = getConditionInZoneConditions(c['name'],
                                                         zone_conditions_data)

                if not condition:
                    sys.exit("Missing zone condition " + c['name'])

                conditions.append(condition)

        zone_group = {}
        zone_group['conditions'] = conditions

        zones = []
        for z in group['zones']:
            zone = {}

            # 'zone' is required
            if ('zone' not in z) or (z['zone'] is None):
                sys.exit("Missing fan zone number in " + zone_yaml)

            zone['num'] = z['zone']

            zone['full_speed'] = z['full_speed']

            zone['default_floor'] = z['default_floor']

            # 'increase_delay' is optional (use 0 by default)
            key = 'increase_delay'
            zone[key] = z.setdefault(key, 0)

            # 'decrease_interval' is optional (use 0 by default)
            key = 'decrease_interval'
            zone[key] = z.setdefault(key, 0)

            # 'cooling_profiles' is optional (use 'all' instead)
            if ('cooling_profiles' not in z) or \
                    (z['cooling_profiles'] is None):
                profiles = ["all"]
            else:
                profiles = z['cooling_profiles']

            # 'interfaces' is optional (no default)
            ifaces = []
            if ('interfaces' in z) and \
                    (z['interfaces'] is not None):
                ifaces = getIfacesInZone(z['interfaces'])

            fans = getFansInZone(z['zone'], profiles, fan_data)
            events = getEventsInZone(z['zone'],
                                     group.get('zone_conditions', {}),
                                     events_data)

            if len(fans) == 0:
                sys.exit("Didn't find any fans in zone " + str(zone['num']))

            if (ifaces):
                zone['ifaces'] = ifaces
            zone['fans'] = fans
            zone['events'] = events
            zones.append(zone)

        zone_group['zones'] = zones
        zone_groups.append(zone_group)

    return zone_groups


if __name__ == '__main__':
    parser = ArgumentParser(
        description="Phosphor fan zone definition parser")

    parser.add_argument('-z', '--zone_yaml', dest='zone_yaml',
                        default="example/zones.yaml",
                        help='fan zone definitional yaml')
    parser.add_argument('-f', '--fan_yaml', dest='fan_yaml',
                        default="example/fans.yaml",
                        help='fan definitional yaml')
    parser.add_argument('-e', '--events_yaml', dest='events_yaml',
                        help='events to set speeds yaml')
    parser.add_argument('-c', '--zone_conditions_yaml',
                        dest='zone_conditions_yaml',
                        help='conditions to determine zone yaml')
    parser.add_argument('-o', '--output_dir', dest='output_dir',
                        default=".",
                        help='output directory')
    args = parser.parse_args()

    if not args.zone_yaml or not args.fan_yaml:
        parser.print_usage()
        sys.exit(1)

    with open(args.zone_yaml, 'r') as zone_input:
        zone_data = yaml.safe_load(zone_input) or {}

    with open(args.fan_yaml, 'r') as fan_input:
        fan_data = yaml.safe_load(fan_input) or {}

    events_data = {}
    if args.events_yaml:
        with open(args.events_yaml, 'r') as events_input:
            events_data = yaml.safe_load(events_input) or {}

    zone_conditions_data = {}
    if args.zone_conditions_yaml:
        with open(args.zone_conditions_yaml, 'r') as zone_conditions_input:
            zone_conditions_data = yaml.safe_load(zone_conditions_input) or {}

    zone_config = buildZoneData(zone_data.get('zone_configuration', {}),
                                fan_data, events_data, zone_conditions_data)

    manager_config = zone_data.get('manager_configuration', {})

    if manager_config.get('power_on_delay') is None:
        manager_config['power_on_delay'] = 0

    tmpls_dir = os.path.join(
        os.path.dirname(os.path.realpath(__file__)),
        "templates")
    output_file = os.path.join(args.output_dir, "fan_zone_defs.cpp")
    if sys.version_info < (3, 0):
        lkup = TemplateLookup(
            directories=tmpls_dir.split(),
            disable_unicode=True)
    else:
        lkup = TemplateLookup(
            directories=tmpls_dir.split())
    tmpl = lkup.get_template('fan_zone_defs.mako.cpp')
    with open(output_file, 'w') as output:
        output.write(tmpl.render(zones=zone_config,
                                 mgr_data=manager_config))
