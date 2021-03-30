#!/usr/bin/python3

# Copyright 2019 IBM Corp.

import yaml
from argparse import ArgumentParser

if __name__ == '__main__':
    # A simple program to ensure proper indents of input yaml file
    parser = ArgumentParser(description="YAML formatter")
    parser.add_argument('-f', '--file', dest='file_yaml',
                        help='yaml file to format')
    args = parser.parse_args()

    print(yaml.dump(yaml.load(open(args.file_yaml),
                              Loader=yaml.FullLoader), indent=4))
