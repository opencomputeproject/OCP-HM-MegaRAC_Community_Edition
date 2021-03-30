#!/usr/bin/python

"""
This script reads in JSON files containing a set of expected entries to be
found within a given input JSON file. An optional "filtering" JSON
file may also be provided that contains a set of data that will be used to
filter configured expected entries from being checked in the input JSON.
"""

import os
import sys
import json
from argparse import ArgumentParser


def findEntry(entry, jsonObject):

    if isinstance(jsonObject, dict):
        for key in jsonObject:
            if key == entry:
                return jsonObject[key]
            else:
                found = findEntry(entry, jsonObject[key])
                if found:
                    return found


def buildDict(entry, jsonObject, resultDict):

    key = list(entry)[0]
    jsonObject = findEntry(key, jsonObject)
    if jsonObject is None:
        return {}
    entry = entry[key]
    if isinstance(entry, dict):
        resultDict[key] = buildDict(entry, jsonObject, resultDict)
    else:
        return {key: jsonObject}


def doAnd(andList, filters):

    allTrue = True
    for entry in andList:
        # $and entries must be atleast a single layer dict
        value = dict()
        buildDict(entry, filters, value)
        if value != entry:
            allTrue = False

    return allTrue


def doOr(orList, filters):

    anyTrue = False
    for entry in orList:
        # $or entries must be atleast a single layer dict
        value = dict()
        buildDict(entry, filters, value)
        if value == entry:
            anyTrue = True
            break

    return anyTrue


def doNor(norList, filters):

    allFalse = True
    for entry in norList:
        # $nor entries must be atleast a single layer dict
        value = dict()
        buildDict(entry, filters, value)
        if value == entry:
            allFalse = False

    return allFalse


def doNot(notDict, filters):

    # $not entry must be atleast a single layer dict
    value = dict()
    buildDict(notDict, filters, value)
    if value == notDict:
        return False

    return True


def applyFilters(expected, filters):

    switch = {
        # $and - Performs an AND operation on an array with at least two
        #        expressions and returns the document that meets all the
        #        expressions. i.e.) {"$and": [{"age": 5}, {"name": "Joe"}]}
        "$and": doAnd,
        # $or - Performs an OR operation on an array with at least two
        #       expressions and returns the documents that meet at least one of
        #       the expressions. i.e.) {"$or": [{"age": 4}, {"name": "Joe"}]}
        "$or": doOr,
        # $nor - Performs a NOR operation on an array with at least two
        #        expressions and returns the documents that do not meet any of
        #        the expressions. i.e.) {"$nor": [{"age": 3}, {"name": "Moe"}]}
        "$nor": doNor,
        # $not - Performs a NOT operation on the specified expression and
        #        returns the documents that do not meet the expression.
        #        i.e.) {"$not": {"age": 4}}
        "$not": doNot
    }

    isExpected = {}
    for entry in expected:
        expectedList = list()
        if entry == "$op":
            addInput = True
            for op in expected[entry]:
                if op != "$input":
                    func = switch.get(op)
                    if not func(expected[entry][op], filters):
                        addInput = False
            if addInput:
                expectedList = expected[entry]["$input"]
        else:
            expectedList = [dict({entry: expected[entry]})]

        for i in expectedList:
            for key in i:
                isExpected[key] = i[key]

    return isExpected


def findExpected(expected, input):

    result = {}
    for key in expected:
        jsonObject = findEntry(key, input)
        if isinstance(expected[key], dict) and expected[key] and jsonObject:
            notExpected = findExpected(expected[key], jsonObject)
            if notExpected:
                result[key] = notExpected
        else:
            # If expected value is not "dont care" and
            # does not equal what's expected
            if str(expected[key]) != "{}" and expected[key] != jsonObject:
                if jsonObject is None:
                    result[key] = None
                else:
                    result[key] = expected[key]
    return result


if __name__ == '__main__':
    parser = ArgumentParser(
        description="Expected JSON cross-checker. Similar to a JSON schema \
                     validator, however this cross-checks a set of expected \
                     property states against the contents of a JSON input \
                     file with the ability to apply an optional set of \
                     filters against what's expected based on the property \
                     states within the provided filter JSON.")

    parser.add_argument('index',
                        help='Index name into a set of entries within the \
                              expected JSON file')
    parser.add_argument('expected_json',
                        help='JSON input file containing the expected set of \
                              entries, by index name, to be contained within \
                              the JSON input file')
    parser.add_argument('input_json',
                        help='JSON input file containing the JSON data to be \
                              cross-checked against what is expected')
    parser.add_argument('-f', '--filters', dest='filter_json',
                        help='JSON file containing path:property:value \
                              associations to optional filters configured \
                              within the expected set of JSON entries')

    args = parser.parse_args()

    with open(args.expected_json, 'r') as expected_json:
        expected = json.load(expected_json) or {}
    with open(args.input_json, 'r') as input_json:
        input = json.load(input_json) or {}

    filters = {}
    if args.filter_json:
        with open(args.filter_json, 'r') as filters_json:
            filters = json.load(filters_json) or {}

    if args.index in expected and expected[args.index] is not None:
        expected = applyFilters(expected[args.index], filters)
        result = findExpected(expected, input)
        if result:
            print("NOT FOUND:")
            for key in result:
                print(key + ": " + str(result[key]))
    else:
        print("Error: " + args.index + " not found in " + args.expected_json)
        sys.exit(1)
