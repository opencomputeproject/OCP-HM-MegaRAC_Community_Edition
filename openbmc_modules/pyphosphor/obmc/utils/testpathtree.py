import unittest

from .pathtree import PathTree

class PathTreeTest(unittest.TestCase):
    def test_set_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1

    def test_set_depth_2(self):
        pt = PathTree()
        pt['/a/b'] = 2

    def test_get_no_key(self):
        pt = PathTree()
        with self.assertRaises(KeyError):
            pt['/foo']

    def test_get_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        self.assertEqual(1, pt['/a'])

    def test_get_depth_2(self):
        pt = PathTree()
        pt['/a/b'] = 2
        self.assertEqual(set(['/a', '/a/b']), set(pt.keys()))
        self.assertEqual(2, pt['/a/b'])

    def test_get_default(self):
        self.assertEquals(1, PathTree().get('/a', 1))

    def test_get_present(self):
        pt = PathTree()
        pt['/a'] = 1
        self.assertEquals(1, pt.get('/a'))

    def test_set_2_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1.1
        pt['/b'] = 1.2
        self.assertEqual(set(['/a', '/b']), set(pt.keys()))
        self.assertEqual(1.1, pt['/a'])
        self.assertEqual(1.2, pt['/b'])

    def test_set_2_depth_2_common_parent(self):
        pt = PathTree()
        pt['/a/b'] = 1.1
        pt['/a/c'] = 1.2
        self.assertEqual(set(['/a', '/a/b', '/a/c']), set(pt.keys()))
        self.assertEqual(1.1, pt['/a/b'])
        self.assertEqual(1.2, pt['/a/c'])

    def test_set_2_depth_2_separate_parent(self):
        pt = PathTree()
        pt['/a/b'] = 1.1
        pt['/b/c'] = 1.2
        self.assertEqual(set(['/a', '/b', '/a/b', '/b/c']), set(pt.keys()))
        self.assertEqual(1.1, pt['/a/b'])
        self.assertEqual(1.2, pt['/b/c'])

    def test_dumpd_empty(self):
        pt = PathTree()
        self.assertEquals(dict(), pt.dumpd())

    def test_dumpd_populated(self):
        pt = PathTree()
        pt['/a/b'] = { 1 : 1.1 }
        pt['/b/c'] = { 2 : 1.2 }
        dump = pt.dumpd()
        self.assertEquals(set(['/']), set(dump.keys()))
        self.assertEquals(set(['a', 'b']), set(dump['/'].keys()))
        self.assertEquals(set(['b']), set(dump['/']['a'].keys()))
        self.assertEquals(set(['c']), set(dump['/']['b'].keys()))

    def test_del_set_1_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        del pt['/a']
        self.assertEquals(0, len(pt.keys()))

    def test_del_set_2_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/b'] = 2
        del pt['/a']
        self.assertEquals(set(['/b']), set(pt.keys()))

    def test_del_set_2_depth_2(self):
        pt = PathTree()
        pt['/a/b'] = 1
        pt['/b/c'] = 2
        del pt['/a/b']
        self.assertEquals(set(['/b/c', '/b']), set(pt.keys()))

    def test_setdefault_present(self):
        pt = PathTree()
        pt['/a'] = 1
        self.assertEquals(1, pt.setdefault('/a', 2))

    def test_setdefault_absent(self):
        self.assertEquals(1, PathTree().setdefault('/a', 1))

    def test_del_no_key(self):
        with self.assertRaises(KeyError):
            del PathTree()['/a']

    def test_values_1(self):
        pt = PathTree()
        pt['/a'] = 1
        self.assertEquals(set([1]), set(pt.values()))

    def test_values_2(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/b'] = 2
        self.assertEquals(set([1, 2]), set(pt.values()))

    def test_items_1(self):
        pt = PathTree()
        pt['/a'] = 1
        self.assertEquals(set([('/a', 1)]), set(pt.items()))

    def test_items_2(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/b'] = 2
        self.assertEquals(set([('/a', 1), ('/b', 2)]), set(pt.items()))

    def test_items_depth_2(self):
        pt = PathTree()
        pt['/a/b'] = 1
        self.assertEquals(set([('/a', None), ('/a/b', 1)]), set(pt.items()))

    def test_dataitems_0(self):
        pt = PathTree()
        self.assertEquals(set(), set(pt.dataitems()))

    def test_dataitems_1(self):
        pt = PathTree()
        pt['/b'] = 1
        self.assertEquals(set([('/b', 1)]), set(pt.dataitems()))

    def test_dataitems_2(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/b/c'] = 2
        self.assertEquals(set([('/a', 1), ('/b/c', 2)]), set(pt.dataitems()))

    def test_get_children(self):
        pt = PathTree()
        pt['/a'] = 1
        self.assertEquals(set(['a']), set(pt.get_children('/')))
        self.assertEquals(set(), set(pt.get_children('/a')))

    def test_get_children_nested(self):
        pt = PathTree()
        pt['/a/b'] = 1
        self.assertEquals(set(['a']), set(pt.get_children('/')))
        self.assertEquals(set(['b']), set(pt.get_children('/a')))
        self.assertEquals(set(), set(pt.get_children('/a/b')))

    def test_demote_1(self):
        pt = PathTree()
        pt['/a'] = 1
        self.assertEquals([1], pt.values())
        pt.demote('/a')
        self.assertEquals([None], pt.values())

    def test_demote_2(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/b'] = 2
        self.assertEquals(set([1, 2]), set(pt.values()))
        pt.demote('/a')
        self.assertEquals(set([None, 2]), set(pt.values()))

    def test_demote_nested(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        self.assertEquals(set([1, 2]), set(pt.values()))
        pt.demote('/a/b')
        self.assertEquals(set([1, None]), set(pt.values()))

    def test_iter(self):
        pt = PathTree()
        pt['/a'] = 1
        i = iter(pt)
        k, v = next(i)
        self.assertEquals('/a', k)
        self.assertEquals(1, v)
        with self.assertRaises(StopIteration):
            next(i)

    def test_iter_2(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/b'] = 2
        i = iter(pt)
        k, v = next(i)
        self.assertEquals('/a', k)
        self.assertEquals(1, v)
        k, v = next(i)
        self.assertEquals('/b', k)
        self.assertEquals(2, v)
        with self.assertRaises(StopIteration):
            next(i)

    def test_iter_2_nested(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        i = iter(pt)
        k, v = next(i)
        self.assertEquals('/a', k)
        self.assertEquals(1, v)
        k, v = next(i)
        self.assertEquals('/a/b', k)
        self.assertEquals(2, v)
        with self.assertRaises(StopIteration):
            next(i)

    def test_keys_2_nested_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        self.assertEquals(set(['/a']), set(pt.keys(depth=1)))

    def test_values_2_nested_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        self.assertEquals(set([1]), set(pt.values(depth=1)))

    def test_items_2_nested_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        self.assertEquals(set([('/a', 1)]), set(pt.items(depth=1)))

    def test_dataitems_2_nested_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/b'] = None
        pt['/b/c'] = 1
        self.assertEquals(set([('/a', 1)]), set(pt.dataitems(depth=1)))

    def test_keys_2_nested_subtree(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/b'] = 3
        self.assertEquals(set(['/a/b']), set(pt.keys(subtree='/a')))

    def test_values_2_nested_subtree(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/b'] = 3
        self.assertEquals(set([2]), set(pt.values(subtree='/a')))

    def test_items_2_nested_subtree(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/b'] = 3
        self.assertEquals(set([('/a/b', 2)]), set(pt.items(subtree='/a')))

    def test_dataitems_2_nested_subtree(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/a/c'] = None
        pt['/b'] = 3
        self.assertEquals(set([('/a/b', 2)]), set(pt.dataitems(subtree='/a')))

    def test_keys_3_nested_subtree_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/a/b/c'] = 3
        pt['/b'] = 4
        self.assertEquals(set(['/a/b']), set(pt.keys(subtree='/a', depth=1)))

    def test_values_3_nested_subtree_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/a/b/c'] = 3
        pt['/b'] = 4
        self.assertEquals(set([2]), set(pt.values(subtree='/a', depth=1)))

    def test_items_3_nested_subtree_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/a/b/c'] = 3
        pt['/b'] = 4
        self.assertEquals(set([('/a/b', 2)]), set(pt.items(subtree='/a', depth=1)))

    def test_items_3_nested_subtree_depth_1(self):
        pt = PathTree()
        pt['/a'] = 1
        pt['/a/b'] = 2
        pt['/a/b/c'] = 3
        pt['/a/c'] = None
        pt['/b'] = 4
        self.assertEquals(set([('/a/b', 2)]), set(pt.dataitems(subtree='/a', depth=1)))

import timeit
import sys

def depth_stress(pt, k):
    pt[k] = 1
    pt[k] = pt[k] + 1
    del pt[k]

depth_setup = """\
from __main__ import depth_stress
from obmc.utils.pathtree import PathTree
pt = PathTree()
key = '/' + '/'.join(['a'] * {})
"""

def width_stress(pt, k):
    pt.get_children(k)

width_setup = """\
from __main__ import width_stress
from obmc.utils.pathtree import PathTree
pt = PathTree()
for i in range(0, {}):
    pt["/{}/a".format(i)] = i
key = '/0/a'
"""

def iter_stress(pt):
    for i in pt.dataitems():
        pass

iter_setup = """\
from __main__ import iter_stress
from obmc.utils.pathtree import PathTree
pt = PathTree()
for i in ('/' + '/'.join([chr(ord('a') + x)] * {}) for x in range(0, 26)):
    for j in range(0, {}):
        k = "{}".format(i, j)
        pt[k] = k
"""

if __name__ == "__main__":
    print("Depth tests:")
    for depth in range(1, 11):
        setup = depth_setup.format(depth)
        stmt = "depth_stress(pt, key)"
        time = timeit.timeit(stmt, setup=setup)
        print("\t{}: {}".format(depth, time))
    print
    print("Width tests:")
    for width in map(lambda x: 2 ** x, range(0, 21)):
        setup = width_setup.format(width, "{}")
        stmt = "width_stress(pt, key)"
        time = timeit.timeit(stmt, setup=setup)
        print("\t{}: {}".format(width, time))
    print
    print("Iteration tests:")
    n = 1000
    for depth in range(1, 5):
        for width in range(1, 5):
            setup = iter_setup.format(depth, width, "{}/{}")
            stmt = "iter_stress(pt)"
            time = timeit.timeit(stmt, setup=setup, number=n)
            print("\tdepth={}, width={}, n={}: {}".format(depth, width, n, time))
