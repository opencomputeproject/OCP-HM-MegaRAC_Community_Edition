#!/usr/bin/python3
# all arguments to this script are considered as json files
# and attempted to be formatted alphabetically

import json
from sys import argv

for file in argv[1:]:
   print("formatting file {}".format(file))
   with open(file) as f:
      j = json.load(f)

   with open(file, 'w') as f:
      f.write(json.dumps(j, indent=4, sort_keys=True, separators=(',', ': ')))
