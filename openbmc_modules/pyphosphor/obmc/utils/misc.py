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


def org_dot_openbmc_match_strings(sep='.', prefix=''):
    matches = [
        ['org', 'openbmc'],
        ['xyz', 'openbmc_project'],
    ]

    return [prefix + sep.join(y) for y in matches]


def org_dot_openbmc_match(name, sep='.', prefix=''):
    names = org_dot_openbmc_match_strings(sep=sep, prefix=prefix)
    return any(
        [x in name or name in x for x in names])


def find_case_insensitive(value, lst):
    return next((x for x in lst if x.lower() == value.lower()), None)


def makelist(data):
    if isinstance(data, list):
            return data
    elif data:
            return [data]
    else:
            return []
