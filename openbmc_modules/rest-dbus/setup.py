from distutils.core import setup
from os import listdir

resources = ['resources/%s' % (x) for x in listdir('resources')]
setup(name='rest-dbus',
      version='1.0',
      scripts=['rest-dbus'],
      data_files=[('rest-dbus/resources', resources)],
      )
