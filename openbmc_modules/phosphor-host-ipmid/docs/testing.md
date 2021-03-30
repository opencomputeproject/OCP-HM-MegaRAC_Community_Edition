# Running Tests

## Setting Up Your Environment

For the purposes of this tutorial, we'll be setting up an environment in Docker.
Docker is handy because it's fairly portable, and won't interfere with the rest
of your machine setup. It also offers a strong guarantee that you're testing
the same way that others working on the project are. Finally, we can get away
with using the same Docker image that's run by the OpenBMC continuous
integration bot, so we have even more confidence that we're running relevant
tests the way they'd be run upstream.

### Install Docker

Installation of Docker CE (Community Edition) varies by platform, and may differ
in your organization. But the
[Docker Docs](https://docs.docker.com/v17.12/install) are a good place to
start looking.

Check that the installation was successful by running `sudo docker run
hello-world`.

### Download OpenBMC Continuous Integration Image

You'll want a couple of different repositories, so start by making a place for
it all to go, and clone the CI scripts:

```shell
mkdir openbmc-ci-tests
cd openbmc-ci-tests
git clone https://github.com/openbmc/openbmc-build-scripts.git
```

### Add `phosphor-host-ipmid`

We also need to put a copy of the project you want to test against here. But
you've probably got a copy checked out already, so we're going to make a
*git worktree*. You can read more about those by running `git help worktree`,
but the basic idea is that it's like having a second copy of your repo - but the
second copy is in sync with your main copy, knows about your local branches, and
protects you from checking out the same branch in two places.

Your new worktree doesn't know about any untracked files you have in your main
worktree, so you should get in the habit of committing everything you want to
run tests against. However, because of the implementation of
`run-unit-test-docker.sh`, you can't run the CI with untracked changes anyways,
so this isn't the worst thing in the world. (If you make untracked changes in
your testing worktree, it's easy to update a commit with those.)

Note the placeholders in the following steps; modify the commands to match your
directory layout.

```shell
cd /my/dir/for/phosphor-host-ipmid
git worktree add /path/to/openbmc-ci-tests/phosphor-host-ipmid
```

Now, if you `cd /path/to/openbmc-ci-tests`, you should see a directory
`phosphor-host-ipmid/`, and if you enter it and run `git status` you will see
that you're likely on a new branch named `phosphor-host-ipmid`. This is just for
convenience, since you can't check out a branch in your worktree that's already
checked out somewhere else; you can safely ignore or delete that branch later.

However, Git won't be able to figure out how to get to your main worktree
(`/my/dir/for/phosphor-host-ipmid`), so we'll need to mount it when we run. Open
up `/path/to/openbmc-ci-tests/openbmc-build-scripts/run-unit-test-docker.sh` and
find where we call `docker run`, way down at the bottom. Add an additional
argument, remembering to escape the newline ('\'):

```shell
PHOSPHOR_IPMI_HOST_PATH="/my/dir/for/phosphor-host-ipmid"

docker run --blah-blah-existing-flags \
  -v ${PHOSPHOR_IPMI_HOST_PATH}:${PHOSPHOR_IPMI_HOST_PATH} \
  -other \
  -args
```

Then commit this, so you can make sure not to lose it if you update the scripts
repo:

```shell
cd openbmc-build-scripts
git add run-unit-test-docker.sh
git commit -m "mount phosphor-host-ipmid"
```

NOTE: There are other ways to do this besides a worktree; other approaches
trade the cruft of mounting extra paths to the Docker container for different
cruft:

You can create a local upstream:
```shell
cd openbmc-ci-tests
mkdir phosphor-host-ipmid
cd phosphor-host-ipmid
git init
cd /my/dir/for/phosphor-host-ipmid
git remote add /path/to/openbmc-ci-tests/phosphor-host-ipmid ci
git push ci
```
This method would require you to push your topic branch to `ci` and then `git
checkout` the appropriate branch every time you switched topics:
```shell
cd /my/dir/for/phosphor-host-ipmid
git commit -m "my changes to be tested"
git push ci
cd /path/to/openbmc-ci-tests/phosphor-host-ipmid
git checkout topic-branch
```

You can also create a symlink from your Git workspace into `openbmc-ci-tests/`.
This is especially not recommended, since you won't be able to work on your code
in parallel while the tests run, and since the CI scripts are unhappy when you
have untracked changes - which you're likely to have during active development.

## Building and Running

The OpenBMC CI scripts take care of the build for you, and run the test suite.
Build and run like so:

```shell
sudo WORKSPACE=$(pwd) UNIT_TEST_PKG=phosphor-host-ipmid \
  ./openbmc-build-scripts/run-unit-test-docker.sh
```

The first run will take a long time! But afterwards it shouldn't be so bad, as
many parts of the Docker container are already downloaded and configured.

## Reading Output

Your results will appear in
`openbmc-ci-tests/phosphor-host-ipmid/test/test-suite.log`, as well as being
printed to `stdout`. You will also see other `.log` files generated for each
test file, for example `sample_unittest.log`. All these `*.log` files are
human-readable and can be examined to determine why something failed

# Writing Tests

Now that you've got an easy working environment for running tests, let's write
some new ones.

## Setting Up Your Environment

In `/my/dir/for/phosphor-host-ipmid`, create a new branch based on `master` (or
your current patchset). For this tutorial, let's call it `sensorhandler-tests`.

```shell
git checkout -b sensorhandler-tests master
```

This creates a new branch `sensorhandler-tests` which is based on `master`, then
checks out that branch for you to start hacking.

## Write Some Tests

For this tutorial, we'll be adding some basic unit testing of the struct
accessors for `get_sdr::GetSdrReq`, just because they're fairly simple. The text
of the struct and accessors is recreated here:

```c++
/**
 * Get SDR
 */
namespace get_sdr
{

struct GetSdrReq
{
    uint8_t reservation_id_lsb;
    uint8_t reservation_id_msb;
    uint8_t record_id_lsb;
    uint8_t record_id_msb;
    uint8_t offset;
    uint8_t bytes_to_read;
} __attribute__((packed));

namespace request
{

inline uint8_t get_reservation_id(GetSdrReq* req)
{
    return (req->reservation_id_lsb + (req->reservation_id_msb << 8));
};

inline uint16_t get_record_id(GetSdrReq* req)
{
    return (req->record_id_lsb + (req->record_id_msb << 8));
};

} // namespace request

...
} // namespace get_sdr
```

We'll create the tests in `test/sensorhandler_unittest.cpp`; go ahead and start
that file with your editor.

First, include the header you want to test, as well as the GTest header:

```c++
#include <sensorhandler.hpp>

#include <gtest/gtest.h>
```

Let's plan the test cases we care about before we build any additional
scaffolding. We've only got two functions - `get_reservation_id()` and
`get_record_id()`. We want to test:

- "Happy path" - in an ideal case, everything works correctly
- Error handling - when given bad input, things break as expected
- Edge cases - when given extremes (e.g. very large or very small numbers),
  things work correctly or break as expected

For `get_reservation_id()`:

```c++
TEST(SensorHandlerTest, GetSdrReq_get_reservation_id_HappyPath)
{
}

TEST(SensorHandlerTest, GetSdrReq_get_reservation_id_NullInputDies)
{
}

TEST(SensorHandlerTest, GetSdrReq_get_reservation_id_Uint16MaxWorksCorrectly)
{
}
```

For `get_record_id()`, we have pretty much the same set of tests:

```c++
TEST(SensorHandlerTest, GetSdrReq_get_record_id_HappyPath)
{
}

TEST(SensorHandlerTest, GetSdrReq_get_record_id_NullInputDies)
{
}

TEST(SensorHandlerTest, GetSdrReq_get_record_id_Uint16MaxWorksCorrectly)
{
}
```

In the case of these two methods, there's really not much else to test. Some
types of edge cases - like overflow/underflow - are prevented by C++'s strong
typing; other types - like passing the incorrect type - are impossible to
insulate against because it's possible to cast anything to a `GetSdrReq*` if we
want. Since these are particularly boring, they make a good example for a
tutorial like this; in practice, tests you write will likely be for more
complicated code! We'll talk more about this in the Best Practices section
below.

Let's implement the `get_reservation_id()` items first. The implementations for
`get_record_id()` will be identical, so we won't cover them here.

For the happy path, we want to make it very clear that the test value we're
using is within range, so we express it in binary. We also want to be able to
ensure that the MSB and LSB are being combined in the correct order, so we make
sure that the MSB and LSB values are different (don't use `0x3333` as the
expected ID here). Finally, we want it to be obvious to the reader if we have
populated the `GetSdrReq` incorrectly, so we've labeled all the fields. Since we
are only testing one operation, it's okay to use either `ASSERT_EQ` or
`EXPECT_EQ`; more on that in the Best Practices section.

```c++
TEST(SensorHandlerTest, GetSdrReq_get_reservation_id_HappyPath)
{
    uint16_t expected_id = 0x1234; // Expected ID spans both bytes.
    GetSdrReq input = {0x34,  // Reservation ID LSB
                       0x12,  // Reservation ID MSB
                       0x00,  // Record ID LSB
                       0x00,  // Record ID MSB
                       0x00,  // Offset
                       0x00}; // Bytes to Read

    uint16_t actual = get_sdr::request::get_reservation_id(&input);
    ASSERT_EQ(actual, expected_id);
}
```

We don't expect that our `GetSdrReq` pointer will ever be null; in this case,
the null pointer validation is done much, much earlier. So it's okay for us to
specify that in the unlikely case we're given a null pointer, we die. We don't
really care what the output message is.

```c++
TEST(SensorHandlerTest, GetSdrReq_get_reservation_id_NullInputDies)
{
    ASSERT_DEATH(get_sdr::request::get_reservation_id(nullptr), ".*");
}
```

Finally, while negative values are taken care of by C++'s type system, we can at
least check that our code still works happily with `UINT16_MAX`. This test is
similar to the happy path test, but uses an edge value instead.

```c++
TEST(SensorHandlerTest, GetSdrReq_get_reservation_id_Uint16MaxWorksCorrectly)
{
    uint16_t expected_id = 0xFFFF; // Expected ID spans both bytes.
    GetSdrReq input = {0xFF,  // Reservation ID LSB
                       0xFF,  // Reservation ID MSB
                       0x00,  // Record ID LSB
                       0x00,  // Record ID MSB
                       0x00,  // Offset
                       0x00}; // Bytes to Read

    uint16_t actual = get_sdr::request::get_reservation_id(&input);
    ASSERT_EQ(actual, expected_id);
}
```

The `get_record_id()` tests are identical, except that they are testing the
Record ID field. They will not be duplicated here.

Finally, we'll need to add the new tests to `test/Makefile.am`. You can mimic other
existing test setups:

```make
# Build/add sensorhandler_unittest to test suite
sensorhandler_unittest_CPPFLAGS = \
    -Igtest \
    $(GTEST_CPPFLAGS) \
    $(AM_CPPFLAGS)
sensorhandler_unittest_CXXFLAGS = \
    $(PTHREAD_CFLAGS) \
    $(CODE_COVERAGE_CXXFLAGS) \
    $(CODE_COVERAGE_CFLAGS) \
    -DBOOST_COROUTINES_NO_DEPRECATION_WARNING
sensorhandler_unittest_LDFLAGS = \
    -lgtest_main \
    -lgtest \
    -pthread \
    $(OESDK_TESTCASE_FLAGS) \
    $(CODE_COVERAGE_LDFLAGS)
sensorhandler_unittest_SOURCES = \
    %reldir%/sensorhandler_unittest.cpp
check_PROGRAMS += %reldir%/sensorhandler_unittest
```

## Run the New Tests

Commit your test changes. Then, you'll want to checkout the
`sensorhandler-tests` branch in your CI worktree, which will involve releasing
it from your main worktree:

```shell
cd /my/dir/for/phosphor-host-ipmid
git add test/sensorhandler_unittest.cpp
git commit -s
git checkout master   # Here you can use any branch except sensorhandler-tests
cd /path/to/openbmc-ci-tests/phosphor-host-ipmid
git checkout sensorhandler-tests
```

Now you can run the test suite as described earlier in the document. If you see
a linter error when you run, you can actually apply the cleaned-up code easily:

```shell
cd ./phosphor-host-ipmid
git diff    # Examine the proposed changes
git add -u  # Apply the proposed changes
git commit --amend
```

(If you will need to apply the proposed changes to multiple commits, you can do
this with interactive rebase, which won't be described here.)

## Best Practices

While a good unit test can ensure your code's stability, a flaky or
poorly-written unit test can make life harder for contributors down the road.
Some things to remember:

Include both positive (happy-path) and negative (error) testing in your
testbench. It's not enough to know that the code works when it's supposed to; we
also need to know that it fails gracefully when something goes wrong. Applying
edge-case testing helps us find bugs that may take years to occur (and
reproduce!) in the field.

Keep your tests small. Avoid branching - if you feel a desire to, instead
explore that codepath in another test. The best tests are easy to read and
understand.

When a test fails, it's useful if the test is named in such a way that you can
tell _what it's supposed to do_ and _when_. That way you can be certain whether
the change you made really broke it or not. A good pattern is
`Object_NameCircumstanceResult` - for example,
`FooFactory_OutOfBoundsNameThrowsException`. From the name, it's very clear that
when some "name" is out of bounds, an exception should be thrown. (What "name"
is should be clear from the context of the function in `FooFactory`.)

Don't test other people's code. Make sure to limit the test assertions to the
code under test. For example, don't test what happens if you give a bad input to
`sdbusplus` when you're supposed to be testing `phosphor-host-ipmid`.

However, don't trust other people's code, either! Try to test _how you respond_
when a service fails unexpectedly. Rather than checking if `sdbusplus` fails on
bad inputs, check whether you handle an `sdbusplus` failure gracefully. You can
use GMock for this kind of testing.

Think about testing when you write your business logic code. Concepts like
dependency injection and small functions make your code more testable - you'll
be thanking yourself later when you're writing tests.

Finally, you're very likely to find bugs while writing tests, especially if it's
for code that wasn't previously unit-tested. It's okay to check in a bugfix
along with a test that verifies the fix worked, if you're only doing one test
and one bugfix. If you're checking in a large suite of tests, do the bugfixes in
independent commits which your test suite commit is based on:

master -> fix Foo.Abc() -> fix Foo.Def() -> Fix Foo.Ghi() -> test Foo class

## Sending for Review

You can send your test update and any associated bugfixes for review to Gerrit
as you would send any other change. For the `Tested:` field in the commit
message, you can state that you ran the new unit tests written.

# Reviewing Tests

Tests are written primarily to be read. So when you review a test, you're the
first customer of that test!

## Best Practices

First, all the best practices listed above for writing tests are things you
should check for and encourage when you're reading tests.

Next, you should ensure that you can tell what's going on when you read the
test. If it's not clear to you, it's not going to be clear to someone else, and
the test is more prone to error - ask!

Finally, think about what's _not_ being tested. If there's a case you're curious
about and it isn't covered, you should mention it to the committer.

## Quickly Running At Home

Now that you've got a handy setup as described earlier in this document, you can
quickly download and run the tests yourself. Within the Gerrit change, you
should be able to find a button that says "Download", which will give you
commands for various types of downloads into an existing Git repo. Use
"Checkout", for example:

```shell
cd openbmc-ci-tests/phosphor-host-ipmid
git fetch "https://gerrit.openbmc-project.xyz/openbmc/phosphor-host-ipmid" \
  refs/changes/43/23043/1 && git checkout FETCH_HEAD
```

This won't disturb the rest of your Git repo state, and will put your CI
worktree into detached-HEAD mode pointing to the commit that's under review. You
can then run your tests normally, and even make changes and push again if the
commit was abandoned or otherwise left to rot by its author.

Doing so can be handy in a number of scenarios:

- Jenkins isn't responding
- The Jenkins build is broken for a reason beyond the committer's control
- The committer doesn't have "Ok-To-Test" permission, and you don't have
  permission to grant it to them

# Credits

Thanks very much to Patrick Venture for his prior work putting together
documentation on this topic internal to Google.
