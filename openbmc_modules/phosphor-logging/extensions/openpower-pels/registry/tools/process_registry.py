#!/usr/bin/env python3

import argparse
import json
import sys

r"""
Validates the PEL message registry JSON, which includes checking it against
a JSON schema using the jsonschema module as well as doing some extra checks
that can't be encoded in the schema.
"""


def check_duplicate_names(registry_json):
    r"""
    Check that there aren't any message registry entries with the same
    'Name' field.  There may be a use case for this in the future, but there
    isn't right now.

    registry_json: The message registry JSON
    """

    names = []
    for entry in registry_json['PELs']:
        if entry['Name'] in names:
            sys.exit("Found multiple uses of error {}".format(entry['Name']))
        else:
            names.append(entry['Name'])


def check_duplicate_reason_codes(registry_json):
    r"""
    Check that there aren't any message registry entries with the same
    'ReasonCode' field.

    registry_json: The message registry JSON
    """

    reasonCodes = []
    for entry in registry_json['PELs']:
        if entry['SRC']['ReasonCode'] in reasonCodes:
            sys.exit("Found duplicate SRC reason code {}".format(
                entry['SRC']['ReasonCode']))
        else:
            reasonCodes.append(entry['SRC']['ReasonCode'])


def check_component_id(registry_json):
    r"""
    Check that the upper byte of the ComponentID field matches the upper byte
    of the ReasonCode field, but not on "11" type SRCs where they aren't
    supposed to match.

    registry_json: The message registry JSON
    """

    for entry in registry_json['PELs']:

        # Don't check on "11" SRCs as those reason codes aren't supposed to
        # match the component ID.
        if entry['SRC'].get('Type', '') == "11":
            continue

        if 'ComponentID' in entry:
            id = int(entry['ComponentID'], 16)
            reason_code = int(entry['SRC']['ReasonCode'], 16)

            if (id & 0xFF00) != (reason_code & 0xFF00):
                sys.exit("Found mismatching component ID {} vs reason "
                         "code {} for error {}".format(
                             entry['ComponentID'],
                             entry['SRC']['ReasonCode'],
                             entry['Name']))


def check_message_args(registry_json):
    r"""
    Check that if the Message field uses the '%' style placeholders that there
    are that many entries in the MessageArgSources field.  Also checks that
    the MessageArgSources field is present but only if there are placeholders.

    registry_json: The message registry JSON
    """

    for entry in registry_json['PELs']:
        num_placeholders = entry['Documentation']['Message'].count('%')
        if num_placeholders == 0:
            continue

        if 'MessageArgSources' not in entry['Documentation']:
            sys.exit("Missing MessageArgSources property for error {}".
                     format(entry['Name']))

        if num_placeholders != \
                len(entry['Documentation']['MessageArgSources']):
                    sys.exit("Different number of placeholders found in "
                             "Message vs MessageArgSources for error {}".
                             format(entry['Name']))


def validate_schema(registry, schema):
    r"""
    Validates the passed in JSON against the passed in schema JSON

    registry: Path of the file containing the registry JSON
    schema:   Path of the file containing the schema JSON
              Use None to skip the pure schema validation
    """

    with open(registry) as registry_handle:
        registry_json = json.load(registry_handle)

        if schema:

            import jsonschema

            with open(schema) as schema_handle:
                schema_json = json.load(schema_handle)

                try:
                    jsonschema.validate(registry_json, schema_json)
                except jsonschema.ValidationError as e:
                    print(e)
                    sys.exit("Schema validation failed")

        check_duplicate_names(registry_json)

        check_duplicate_reason_codes(registry_json)

        check_component_id(registry_json)

        check_message_args(registry_json)


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='PEL message registry processor')

    parser.add_argument('-v', '--validate', action='store_true',
                        dest='validate',
                        help='Validate the JSON using the schema')

    parser.add_argument('-s', '--schema-file', dest='schema_file',
                        help='The message registry JSON schema file')

    parser.add_argument('-r', '--registry-file', dest='registry_file',
                        help='The message registry JSON file')
    parser.add_argument('-k', '--skip-schema-validation', action='store_true',
                        dest='skip_schema',
                        help='Skip running schema validation. '
                             'Only do the extra checks.')

    args = parser.parse_args()

    if args.validate:
        if not args.schema_file:
            sys.exit("Schema file required")

        if not args.registry_file:
            sys.exit("Registry file required")

        schema = args.schema_file
        if args.skip_schema:
            schema = None

        validate_schema(args.registry_file, schema)
