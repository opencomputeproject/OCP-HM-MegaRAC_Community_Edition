# Contributors Listed Below - COPYRIGHT 2016
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import json
import os
# TODO: openbmc/openbmc#2994 remove python 2 support
import sys
if sys.version_info[0] < 3:
    import cPickle as pickle
else:
    import pickle

CACHE_PATH = '/var/cache/obmc/'


def getCacheFilename(obj_path, iface_name):
    name = obj_path.replace('/', '.')
    filename = CACHE_PATH + name[1:] + "@" + iface_name + ".props"
    return filename


def save(obj_path, iface_name, properties):
    print("Caching: "+ obj_path)
    filename = getCacheFilename(obj_path, iface_name)
    parent = os.path.dirname(filename)
    try:
        if not os.path.exists(parent):
            os.makedirs(parent)
        with open(filename, 'wb') as output:
            try:
                # use json module to convert dbus datatypes
                props = json.dumps(properties[iface_name])
                prop_obj = json.loads(props)
                pickle.dump(prop_obj, output)
            except Exception as e:
                print("ERROR: " + str(e))
    except Exception:
        print("ERROR opening cache file: " + filename)


def load(obj_path, iface_name, properties):
    # overlay with pickled data
    filename = getCacheFilename(obj_path, iface_name)
    if (os.path.isfile(filename)):
        if iface_name in properties:
            properties[iface_name] = {}
        print("Loading from cache: " + filename)
        try:
            p = open(filename, 'rb')
            data = pickle.load(p)
            for prop in list(data.keys()):
                properties[iface_name][prop] = data[prop]

        except Exception as e:
            print("ERROR: Loading cache file: " + str(e))
        finally:
            p.close()
