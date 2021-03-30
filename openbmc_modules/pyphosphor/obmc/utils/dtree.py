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


def dts_encode(obj, fd, **kw):
    ''' A rudimentary python to dts encoder.
    '''
    indent = kw.get('indent', 0)
    depth = kw.setdefault('depth', 0)
    tab = indent * depth * ' '
    kw['depth'] += 1
    newline = '\n' if indent else ' '
    context = kw.get('context')

    if(isinstance(obj, dict)):
        nodes = []
        for k, v in obj.iteritems():
            if(isinstance(v, dict)):
                nodes.append((k, v))
                continue
            if(isinstance(v, basestring) and v.lower() == 'true'):
                fd.write('%s%s' % (tab, k))
            elif(isinstance(v, basestring) and v.lower() == 'false'):
                continue
            else:
                fd.write('%s%s = ' % (tab, k))
                dts_encode(v, fd, **kw)
            fd.write(";%s" % newline)

        for k, v in nodes:
            fd.write('%s%s {%s' % (tab, k, newline))
            dts_encode(v, fd, **kw)
            fd.write('%s};%s' % (tab, newline))

    if(isinstance(obj, int)):
        if context == 'int_list':
            fd.write("%d" % obj)
        else:
            fd.write("<%d>" % obj)

    if(isinstance(obj, basestring)):
        fd.write("\"%s\"" % obj)

    if(isinstance(obj, list)):
        ctx = 'int_list' if all((type(x) is int) for x in iter(obj)) else ''
        if ctx is 'int_list':
            delim = ' '
            closure = ('<', '>')
        else:
            delim = ','
            closure = ('', '')

        fd.write(closure[0])
        if obj:
            for v in obj[:-1]:
                dts_encode(v, fd, context=ctx, **kw)
                fd.write(delim)

            dts_encode(obj[-1], fd, context=ctx)

        fd.write(closure[1])
