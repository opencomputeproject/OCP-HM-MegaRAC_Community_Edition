#!/usr/bin/env python3

from distutils.core import setup

setup(name='obmc-gerrit',
      version='0.1',
      description='OpenBMC Gerrit wrapper',
      author='Andrew Jeffery',
      author_email='andrew@aj.id.au',
      url='https://github.com/openbmc/openbmc-tools',
      packages=['obmc'],
      requires=['requests', 'sh'],
      scripts=['obmc-gerrit'],
     )
