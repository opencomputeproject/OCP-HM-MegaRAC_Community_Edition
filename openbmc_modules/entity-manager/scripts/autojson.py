#!/usr/bin/python3
# all arguments to this script are considered as json files
# and attempted to be formatted alphabetically

import json
import os
from sys import argv

files = argv[1:]

for file in files[:]:
    if os.path.isdir(file):
        files.remove(file)
        for f in os.listdir(file):
            files.append(os.path.join(file, f))

for file in files:
    if not file.endswith('.json'):
        continue
    print("formatting file {}".format(file))
    with open(file) as f:
        j = json.load(f)

    if isinstance(j, list):
        for item in j:
            item["Exposes"] = sorted(item["Exposes"], key=lambda k: k["Type"])
    else:
        j["Exposes"] = sorted(j["Exposes"], key=lambda k: k["Type"])

    with open(file, 'w') as f:
        f.write(json.dumps(j, indent=4, sort_keys=True, separators=(',', ': ')))
