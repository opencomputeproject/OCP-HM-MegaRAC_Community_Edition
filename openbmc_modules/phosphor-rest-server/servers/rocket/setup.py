from distutils.core import setup

setup(
    name='phosphor-rocket',
    version='1.0',
    scripts=['phosphor-rocket'],
    data_files=[('phosphor-rocket', ['cert.pem'])],
    )
