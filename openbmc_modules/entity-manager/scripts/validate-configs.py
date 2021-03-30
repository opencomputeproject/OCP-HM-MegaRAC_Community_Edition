#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""
A tool for validating entity manager configurations.
"""
import argparse
import json
import jsonschema.validators
import os
import sys

DEFAULT_SCHEMA_FILENAME = "global.json"


def main():
    parser = argparse.ArgumentParser(
        description="Entity manager configuration validator",
    )
    parser.add_argument(
        "-s", "--schema", help=(
            "Use the specified schema file instead of the default "
            "(__file__/../../schemas/global.json)"))
    parser.add_argument(
        "-c", "--config", action='append', help=(
            "Validate the specified configuration files (can be "
            "specified more than once) instead of the default "
            "(__file__/../../configurations/**.json)"))
    parser.add_argument(
        "-e", "--expected-fails", help=(
            "A file with a list of configurations to ignore should "
            "they fail to validate"))
    parser.add_argument(
        "-k", "--continue", action='store_true', help=(
            "keep validating after a failure"))
    parser.add_argument(
        "-v", "--verbose", action='store_true', help=(
            "be noisy"))
    args = parser.parse_args()

    schema_file = args.schema
    if schema_file is None:
        try:
            source_dir = os.path.realpath(__file__).split(os.sep)[:-2]
            schema_file = os.sep + os.path.join(
                *source_dir, 'schemas', DEFAULT_SCHEMA_FILENAME)
        except Exception as e:
            sys.stderr.write(
                "Could not guess location of {}\n".format(
                    DEFAULT_SCHEMA_FILENAME))
            sys.exit(2)

    schema = {}
    try:
        with open(schema_file) as fd:
            schema = json.load(fd)
    except FileNotFoundError as e:
        sys.stderr.write(
            "Could not read schema file '{}'\n".format(schema_file))
        sys.exit(2)

    config_files = args.config or []
    if len(config_files) == 0:
        try:
            source_dir = os.path.realpath(__file__).split(os.sep)[:-2]
            configs_dir = os.sep + os.path.join(*source_dir, 'configurations')
            data = os.walk(configs_dir)
            for root, _, files in data:
                for f in files:
                    if f.endswith('.json'):
                        config_files.append(os.path.join(root, f))
        except Exception as e:
            sys.stderr.write(
                "Could not guess location of configurations\n")
            sys.exit(2)

    configs = []
    for config_file in config_files:
        try:
            with open(config_file) as fd:
                configs.append(json.load(fd))
        except FileNotFoundError as e:
            sys.stderr.write(
                    "Could not parse config file '{}'\n".format(config_file))
            sys.exit(2)

    expected_fails = []
    if args.expected_fails:
        try:
            with open(args.expected_fails) as fd:
                for line in fd:
                    expected_fails.append(line.strip())
        except Exception as e:
            sys.stderr.write(
                    "Could not read expected fails file '{}'\n".format(
                        args.expected_fails))
            sys.exit(2)

    base_uri = "file://{}/".format(
        os.path.split(os.path.realpath(schema_file))[0])
    resolver = jsonschema.RefResolver(base_uri, schema)
    validator = jsonschema.Draft7Validator(schema, resolver=resolver)

    results = {
        "invalid": [],
        "unexpected_pass": [],
    }
    for config_file, config in zip(config_files, configs):
        name = os.path.split(config_file)[1]
        expect_fail = name in expected_fails
        try:
            validator.validate(config)
            if expect_fail:
                results["unexpected_pass"].append(name)
                if not getattr(args, "continue"):
                    break
        except jsonschema.exceptions.ValidationError as e:
            if not expect_fail:
                results["invalid"].append(name)
                if args.verbose:
                    print(e)
            if expect_fail or getattr(args, "continue"):
                continue
            break

    exit_status = 0
    if len(results["invalid"]) + len(results["unexpected_pass"]):
        exit_status = 1
        unexpected_pass_suffix = " **"
        show_suffix_explanation = False
        print("results:")
        for f in config_files:
            if any([x in f for x in results["unexpected_pass"]]):
                show_suffix_explanation = True
                print("  '{}' passed!{}".format(f, unexpected_pass_suffix))
            if any([x in f for x in results["invalid"]]):
                print("  '{}' failed!".format(f))

        if show_suffix_explanation:
            print("\n** configuration expected to fail")

    sys.exit(exit_status)


if __name__ == "__main__":
    main()
