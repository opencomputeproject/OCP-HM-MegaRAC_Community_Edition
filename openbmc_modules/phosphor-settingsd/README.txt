How to use 'query' in the YAML file
'query' contains information for settings_manager.py to search for
and match inventory objects:

query:
    name: type
    keyregex: "dimm"
    subtree: "/org/openbmc/inventory"
    matchregex: "/(dimm\d*)$"

In the example above setting_manager.py will explore all existing
objects at /org/openbmc/inventory that relate to the 'dimm' main
category in the YAML file.
The 'matchregex' will identify all objects with names that start with 'dimm',
followed by any number of digits. The name should be at the end of the path.
settings_manager.py will create a corresponding object with the same name,
which is a group (dimm\d*).
