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


class PathTreeItemIterator(object):
    def __init__(self, path_tree, subtree, depth):
        self.path_tree = path_tree
        self.path = []
        self.itlist = []
        self.subtree = ['/'] + list(filter(bool, subtree.split('/')))
        self.depth = depth
        d = path_tree.root
        for k in self.subtree:
            try:
                d = d[k]['children']
            except KeyError:
                raise KeyError(subtree)
        # TODO: openbmc/openbmc#2994 remove python 2 support
        try:  # python 2
            self.it = d.iteritems()
        except AttributeError:  # python 3
            self.it = iter(d.items())

    def __iter__(self):
        return self

    # TODO: openbmc/openbmc#2994 remove python 2 support
    # python 2
    def next(self):
        key, value = self._next()
        path = self.subtree[0] + '/'.join(self.subtree[1:] + self.path)
        return path, value.get('data')

    # python 3
    import sys
    if sys.version_info[0] > 2:
        __next__ = next

    def _next(self):
        try:
            while True:
                x = next(self.it)
                if self.depth:
                    if len(self.path) + 1 > self.depth:
                        continue
                self.itlist.append(self.it)
                self.path.append(x[0])
                # TODO: openbmc/openbmc#2994 remove python 2 support
                try:  # python 2
                    self.it = x[1]['children'].iteritems()
                except AttributeError:  # python 3
                    self.it = iter(x[1]['children'].items())
                break

        except StopIteration:
            if not self.itlist:
                raise StopIteration

            self.it = self.itlist.pop()
            self.path.pop()
            x = self._next()

        return x


class PathTreeKeyIterator(PathTreeItemIterator):
    def __init__(self, path_tree, subtree, depth):
        super(PathTreeKeyIterator, self).__init__(path_tree, subtree, depth)

    # TODO: openbmc/openbmc#2994 remove python 2 support
    # python 2
    def next(self):
        return super(PathTreeKeyIterator, self).next()[0]

    # python 3
    import sys
    if sys.version_info[0] > 2:
        __next__ = next


class PathTree:
    def __init__(self):
        self.root = {}
        self.cache = {}

    def _try_delete_parent(self, elements):
        if len(elements) == 1:
            return False

        kids = 'children'
        elements.pop()
        d = self.root
        for k in elements[:-1]:
            d = d[k][kids]

        if 'data' not in d[elements[-1]] and not d[elements[-1]][kids]:
            del d[elements[-1]]
            self._try_delete_parent(elements)

    def _get_node(self, key):
        kids = 'children'
        elements = ['/'] + list(filter(bool, key.split('/')))
        d = self.root
        for k in elements[:-1]:
            try:
                d = d[k][kids]
            except KeyError:
                raise KeyError(key)

        return d[elements[-1]]

    def __iter__(self):
        return PathTreeItemIterator(self, '/', None)

    def __missing__(self, key):
        for x in self.iterkeys():
            if key == x:
                return False
        return True

    def __delitem__(self, key):
        del self.cache[key]
        kids = 'children'
        elements = ['/'] + list(filter(bool, key.split('/')))
        d = self.root
        for k in elements[:-1]:
            try:
                d = d[k][kids]
            except KeyError:
                raise KeyError(key)

        del d[elements[-1]]
        self._try_delete_parent(elements)

    def __setitem__(self, key, value):
        self.cache[key] = value
        kids = 'children'
        elements = ['/'] + list(filter(bool, key.split('/')))
        d = self.root
        for k in elements[:-1]:
            d = d.setdefault(k, {kids: {}})[kids]

        children = d.setdefault(elements[-1], {kids: {}})[kids]
        d[elements[-1]].update({kids: children, 'data': value})

    def __getitem__(self, key):
        if key in self.cache:
            return self.cache[key]

        return self._get_node(key).get('data')

    def setdefault(self, key, default):
        if not self.get(key):
            self.__setitem__(key, default)

        return self.__getitem__(key)

    def get(self, key, default=None):
        try:
            x = self.__getitem__(key)
        except KeyError:
            x = default

        return x

    def get_children(self, key):
        return [x for x in self._get_node(key)['children'].keys()]

    def demote(self, key):
        n = self._get_node(key)
        if 'data' in n:
            del n['data']

    def keys(self, subtree='/', depth=None):
        return [x for x in self.iterkeys(subtree, depth)]

    def values(self, subtree='/', depth=None):
        return [x[1] for x in self.iteritems(subtree, depth)]

    def items(self, subtree='/', depth=None):
        return [x for x in self.iteritems(subtree, depth)]

    def dataitems(self, subtree='/', depth=None):
        # dataitems() must return an iterable object containing all of the
        # items explicitly inserted into the tree, rooted at subtree with
        # depth number of path elements from the subtree root.
        #
        # Calling dataitems() to request all of the populated entries is
        # unfortunately common, and it's (also) unfortunately expensive to
        # generate (done by iterating over the entire tree). However, given we
        # have a flat dict whose job is to cache the explicitly inserted values
        # we can leverage that to provide a cheap-to-calculate answer to
        # requests for the entire set of populated entries
        if subtree == '/' and not depth:
            return self.cache.items()

        return [x for x in self.iteritems(subtree, depth)
                if x[1] is not None]

    def iterkeys(self, subtree='/', depth=None):
        if not self.root:
            # TODO: openbmc/openbmc#2994 remove python 2 support
            try:  # python 2
                return {}.iterkeys()
            except AttributeError:  # python 3
                return iter({}.keys())
        return PathTreeKeyIterator(self, subtree, depth)

    def iteritems(self, subtree='/', depth=None):
        if not self.root:
            # TODO: openbmc/openbmc#2994 remove python 2 support
            try:  # python 2
                return {}.iteritems()
            except AttributeError:  # python 3
                return iter({}.items())
        return PathTreeItemIterator(self, subtree, depth)

    def dumpd(self, subtree='/'):
        result = {}
        d = result

        for k, v in self.iteritems(subtree):
            elements = ['/'] + list(filter(bool, k.split('/')))
            d = result
            for k in elements:
                d = d.setdefault(k, {})
            if v is not None:
                d.update(v)

        return result
