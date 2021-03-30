Instructions on how to execute UTEST.

* When using an SDK - make sure it has been built
  for an x86 machine.

- First generate the configure script with a call
  to ./bootstrap.sh.
- Once built, configure flags with
  "./configure ${CONFIGURE_FLAGS} --enable-oe-sdk"
- Be sure to include --enable-oe-sdk or the tests
  will not run properly.
- Lastly "make check" will build the application
  source along with unit tests.  Make check also
  runs the unit tests automatically.

* WHEN RUNNING UTEST remember to take advantage
  of the gtest capabilities. "./utest --help"
  - --gtest_repeat=[COUNT]
  - --gtest_shuffle
  - --gtest_random_seed=[NUMBER]
