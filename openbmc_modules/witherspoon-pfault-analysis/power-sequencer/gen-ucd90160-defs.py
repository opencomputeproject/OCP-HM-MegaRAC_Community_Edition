#!/usr/bin/env python3

import os
import yaml
from argparse import ArgumentParser
from mako.template import Template
from mako.lookup import TemplateLookup

if __name__ == '__main__':
    parser = ArgumentParser(
        description="Power sequencer UCD90160 definition parser")

    parser.add_argument('-i', '--input_yaml', dest='input_yaml',
                        default="example/ucd90160.yaml",
                        help='UCD90160 definitions YAML')

    parser.add_argument('-o', '--output_dir', dest='output_dir',
                        default=".",
                        help='output directory')

    args = parser.parse_args()

    if not args.input_yaml or not args.output_dir:
        parser.print_usage()
        sys.exit(1)

    with open(args.input_yaml, 'r') as ucd90160_input:
        ucd90160_data = yaml.safe_load(ucd90160_input) or {}

    templates_dir = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "templates")

    output_file = os.path.join(args.output_dir, "ucd90160_defs.cpp")

    mylookup = TemplateLookup(
            directories=templates_dir.split())
    mytemplate = mylookup.get_template('ucd90160_defs.mako.cpp')

    with open(output_file, 'w') as output:
        output.write(mytemplate.render(ucd90160s=ucd90160_data))
