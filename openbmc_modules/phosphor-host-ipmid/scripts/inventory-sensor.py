#!/usr/bin/env python3

import os
import sys
import yaml
import argparse
from mako.template import Template


def generate_cpp(sensor_yaml, output_dir):
    with open(sensor_yaml, 'r') as f:
        ifile = yaml.safe_load(f)
        if not isinstance(ifile, dict):
            ifile = {}

        # Render the mako template

        t = Template(filename=os.path.join(
                     script_dir,
                     "inventorysensor.mako.cpp"))

        output_cpp = os.path.join(output_dir, "inventory-sensor-gen.cpp")
        with open(output_cpp, 'w') as fd:
            fd.write(t.render(sensorDict=ifile))


def main():

    valid_commands = {
        'generate-cpp': generate_cpp
    }
    parser = argparse.ArgumentParser(
        description="Inventory Object to IPMI SensorID code generator")

    parser.add_argument(
        '-i', '--sensor_yaml', dest='sensor_yaml',
        default='example.yaml', help='input sensor yaml file to parse')

    parser.add_argument(
        "-o", "--output-dir", dest="outputdir",
        default=".",
        help="output directory")

    parser.add_argument(
        'command', metavar='COMMAND', type=str,
        choices=valid_commands.keys(),
        help='Command to run.')

    args = parser.parse_args()

    if (not (os.path.isfile(args.sensor_yaml))):
        sys.exit("Can not find input yaml file " + args.sensor_yaml)

    function = valid_commands[args.command]
    function(args.sensor_yaml, args.outputdir)


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
