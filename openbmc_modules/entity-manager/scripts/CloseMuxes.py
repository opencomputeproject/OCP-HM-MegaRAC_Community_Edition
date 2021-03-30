#!/usr/bin/python
"""
This script sets muxes back to default values so that
during device parsing we are not left with an invisible
mux before the mux has been added to device tree.

If we find a better way to take care of this, we should
remove this file.
"""

import os
import json
import subprocess
import re

CONFIGURATION_FILE = '/var/configuration/system.json'

if not os.path.isfile(CONFIGURATION_FILE):
    print('No Configuration')
    exit(0)

configuration = json.load(open(CONFIGURATION_FILE))

for _, entity in configuration.iteritems():
    for exposed in entity.get('Exposes', []):
        if re.match("PCA954\\dMux", exposed.get('Type', None)):
            bus = exposed.get('Bus', False)
            address = exposed.get('Address', False)
            if bus and address:
                subprocess.call('i2cset -y -f {} {} 0'.format(bus, address),
                                shell=True)
