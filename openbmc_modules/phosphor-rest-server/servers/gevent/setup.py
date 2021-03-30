from distutils.core import setup

setup(
    name='phosphor-gevent',
    version='1.0',
    scripts=['phosphor-gevent'],
    data_files=[('phosphor-gevent', ['cert.pem'])],
    )
