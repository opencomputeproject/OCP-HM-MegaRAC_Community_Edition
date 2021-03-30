# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import os
import re

from distutils.core import setup


def get_version():
    base = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(base, 'fru.py'), 'r') as f:
        blob = f.read()
    return re.search(r"__version__ = '(.+)'", blob).group(1)


setup(
    name='fru',
    version=get_version(),
    url='https://github.com/genotrance/fru-tool/',
    author='Ganesh Viswanathan',
    license='MIT',
    py_modules=['fru'],
    long_description='Generate binary IPMI FRU data files',
)
