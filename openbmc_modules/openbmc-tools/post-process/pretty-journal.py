#!/usr/bin/env python

r"""
BMC FFDC will at times include the journal in json format
(journalctl -o json-pretty ).  This is a quick and dirty script which
will convert that json output into the standard journalctl output
"""

import json
import re
import time
from argparse import ArgumentParser


def jpretty_to_python(buf):
    entries = []

    for entry in re.findall(
            '^{$(.+?)^}$', buf, re.DOTALL | re.MULTILINE):
        entries += [json.loads('{{{}}}'.format(entry))]

    return entries


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument(
        'journalfile', metavar='FILE', help='the file to parse')

    args = parser.parse_args()

    with open(args.journalfile) as fd:
        entries = jpretty_to_python(fd.read())
        entries = sorted(entries, key=lambda k: k['__REALTIME_TIMESTAMP'])

        for e in entries:
            e['ts'] = time.ctime(
                float(e['__REALTIME_TIMESTAMP']) / 1000000).rstrip()
            try:
                print ('{ts} {_HOSTNAME} {SYSLOG_IDENTIFIER}: {MESSAGE}'.format(**e))
            except:
                print ("Unable to parse msg: " + str(e))
                continue
