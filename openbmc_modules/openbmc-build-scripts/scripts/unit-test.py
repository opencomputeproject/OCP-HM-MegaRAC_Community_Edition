#!/usr/bin/env python3

"""
This script determines the given package's openbmc dependencies from its
configure.ac file where it downloads, configures, builds, and installs each of
these dependencies. Then the given package is configured, built, and installed
prior to executing its unit tests.
"""

from git import Repo
from mesonbuild import coredata, optinterpreter
from urllib.parse import urljoin
from subprocess import check_call, call, CalledProcessError
import os
import sys
import argparse
import multiprocessing
import re
import subprocess
import shutil
import platform


class DepTree():
    """
    Represents package dependency tree, where each node is a DepTree with a
    name and DepTree children.
    """

    def __init__(self, name):
        """
        Create new DepTree.

        Parameter descriptions:
        name               Name of new tree node.
        """
        self.name = name
        self.children = list()

    def AddChild(self, name):
        """
        Add new child node to current node.

        Parameter descriptions:
        name               Name of new child
        """
        new_child = DepTree(name)
        self.children.append(new_child)
        return new_child

    def AddChildNode(self, node):
        """
        Add existing child node to current node.

        Parameter descriptions:
        node               Tree node to add
        """
        self.children.append(node)

    def RemoveChild(self, name):
        """
        Remove child node.

        Parameter descriptions:
        name               Name of child to remove
        """
        for child in self.children:
            if child.name == name:
                self.children.remove(child)
                return

    def GetNode(self, name):
        """
        Return node with matching name. Return None if not found.

        Parameter descriptions:
        name               Name of node to return
        """
        if self.name == name:
            return self
        for child in self.children:
            node = child.GetNode(name)
            if node:
                return node
        return None

    def GetParentNode(self, name, parent_node=None):
        """
        Return parent of node with matching name. Return none if not found.

        Parameter descriptions:
        name               Name of node to get parent of
        parent_node        Parent of current node
        """
        if self.name == name:
            return parent_node
        for child in self.children:
            found_node = child.GetParentNode(name, self)
            if found_node:
                return found_node
        return None

    def GetPath(self, name, path=None):
        """
        Return list of node names from head to matching name.
        Return None if not found.

        Parameter descriptions:
        name               Name of node
        path               List of node names from head to current node
        """
        if not path:
            path = []
        if self.name == name:
            path.append(self.name)
            return path
        for child in self.children:
            match = child.GetPath(name, path + [self.name])
            if match:
                return match
        return None

    def GetPathRegex(self, name, regex_str, path=None):
        """
        Return list of node paths that end in name, or match regex_str.
        Return empty list if not found.

        Parameter descriptions:
        name               Name of node to search for
        regex_str          Regex string to match node names
        path               Path of node names from head to current node
        """
        new_paths = []
        if not path:
            path = []
        match = re.match(regex_str, self.name)
        if (self.name == name) or (match):
            new_paths.append(path + [self.name])
        for child in self.children:
            return_paths = None
            full_path = path + [self.name]
            return_paths = child.GetPathRegex(name, regex_str, full_path)
            for i in return_paths:
                new_paths.append(i)
        return new_paths

    def MoveNode(self, from_name, to_name):
        """
        Mode existing from_name node to become child of to_name node.

        Parameter descriptions:
        from_name          Name of node to make a child of to_name
        to_name            Name of node to make parent of from_name
        """
        parent_from_node = self.GetParentNode(from_name)
        from_node = self.GetNode(from_name)
        parent_from_node.RemoveChild(from_name)
        to_node = self.GetNode(to_name)
        to_node.AddChildNode(from_node)

    def ReorderDeps(self, name, regex_str):
        """
        Reorder dependency tree.  If tree contains nodes with names that
        match 'name' and 'regex_str', move 'regex_str' nodes that are
        to the right of 'name' node, so that they become children of the
        'name' node.

        Parameter descriptions:
        name               Name of node to look for
        regex_str          Regex string to match names to
        """
        name_path = self.GetPath(name)
        if not name_path:
            return
        paths = self.GetPathRegex(name, regex_str)
        is_name_in_paths = False
        name_index = 0
        for i in range(len(paths)):
            path = paths[i]
            if path[-1] == name:
                is_name_in_paths = True
                name_index = i
                break
        if not is_name_in_paths:
            return
        for i in range(name_index + 1, len(paths)):
            path = paths[i]
            if name in path:
                continue
            from_name = path[-1]
            self.MoveNode(from_name, name)

    def GetInstallList(self):
        """
        Return post-order list of node names.

        Parameter descriptions:
        """
        install_list = []
        for child in self.children:
            child_install_list = child.GetInstallList()
            install_list.extend(child_install_list)
        install_list.append(self.name)
        return install_list

    def PrintTree(self, level=0):
        """
        Print pre-order node names with indentation denoting node depth level.

        Parameter descriptions:
        level              Current depth level
        """
        INDENT_PER_LEVEL = 4
        print(' ' * (level * INDENT_PER_LEVEL) + self.name)
        for child in self.children:
            child.PrintTree(level + 1)


def check_call_cmd(*cmd):
    """
    Verbose prints the directory location the given command is called from and
    the command, then executes the command using check_call.

    Parameter descriptions:
    dir                 Directory location command is to be called from
    cmd                 List of parameters constructing the complete command
    """
    printline(os.getcwd(), ">", " ".join(cmd))
    check_call(cmd)


def clone_pkg(pkg, branch):
    """
    Clone the given openbmc package's git repository from gerrit into
    the WORKSPACE location

    Parameter descriptions:
    pkg                 Name of the package to clone
    branch              Branch to clone from pkg
    """
    pkg_dir = os.path.join(WORKSPACE, pkg)
    if os.path.exists(os.path.join(pkg_dir, '.git')):
        return pkg_dir
    pkg_repo = urljoin('https://gerrit.openbmc-project.xyz/openbmc/', pkg)
    os.mkdir(pkg_dir)
    printline(pkg_dir, "> git clone", pkg_repo, branch, "./")
    try:
        # first try the branch
        clone = Repo.clone_from(pkg_repo, pkg_dir, branch=branch)
        repo_inst = clone.working_dir
    except:
        printline("Input branch not found, default to master")
        clone = Repo.clone_from(pkg_repo, pkg_dir, branch="master")
        repo_inst = clone.working_dir
    return repo_inst


def make_target_exists(target):
    """
    Runs a check against the makefile in the current directory to determine
    if the target exists so that it can be built.

    Parameter descriptions:
    target              The make target we are checking
    """
    try:
        cmd = ['make', '-n', target]
        with open(os.devnull, 'w') as devnull:
            check_call(cmd, stdout=devnull, stderr=devnull)
        return True
    except CalledProcessError:
        return False


make_parallel = [
    'make',
    # Run enough jobs to saturate all the cpus
    '-j', str(multiprocessing.cpu_count()),
    # Don't start more jobs if the load avg is too high
    '-l', str(multiprocessing.cpu_count()),
    # Synchronize the output so logs aren't intermixed in stdout / stderr
    '-O',
]


def build_and_install(name, build_for_testing=False):
    """
    Builds and installs the package in the environment. Optionally
    builds the examples and test cases for package.

    Parameter description:
    name                The name of the package we are building
    build_for_testing   Enable options related to testing on the package?
    """
    os.chdir(os.path.join(WORKSPACE, name))

    # Refresh dynamic linker run time bindings for dependencies
    check_call_cmd('sudo', '-n', '--', 'ldconfig')

    pkg = Package()
    if build_for_testing:
        pkg.test()
    else:
        pkg.install()


def build_dep_tree(name, pkgdir, dep_added, head, branch, dep_tree=None):
    """
    For each package (name), starting with the package to be unit tested,
    extract its dependencies. For each package dependency defined, recursively
    apply the same strategy

    Parameter descriptions:
    name                Name of the package
    pkgdir              Directory where package source is located
    dep_added           Current dict of dependencies and added status
    head                Head node of the dependency tree
    branch              Branch to clone from pkg
    dep_tree            Current dependency tree node
    """
    if not dep_tree:
        dep_tree = head

    with open("/tmp/depcache", "r") as depcache:
        cache = depcache.readline()

    # Read out pkg dependencies
    pkg = Package(name, pkgdir)

    for dep in set(pkg.build_system().dependencies()):
        if dep in cache:
            continue
        # Dependency package not already known
        if dep_added.get(dep) is None:
            # Dependency package not added
            new_child = dep_tree.AddChild(dep)
            dep_added[dep] = False
            dep_pkgdir = clone_pkg(dep, branch)
            # Determine this dependency package's
            # dependencies and add them before
            # returning to add this package
            dep_added = build_dep_tree(dep,
                                       dep_pkgdir,
                                       dep_added,
                                       head,
                                       branch,
                                       new_child)
        else:
            # Dependency package known and added
            if dep_added[dep]:
                continue
            else:
                # Cyclic dependency failure
                raise Exception("Cyclic dependencies found in "+name)

    if not dep_added[name]:
        dep_added[name] = True

    return dep_added


def run_cppcheck():
    match_re = re.compile(r'((?!\.mako\.).)*\.[ch](?:pp)?$', re.I)
    cppcheck_files = []
    stdout = subprocess.check_output(['git', 'ls-files'])

    for f in stdout.decode('utf-8').split():
        if match_re.match(f):
            cppcheck_files.append(f)

    if not cppcheck_files:
        # skip cppcheck if there arent' any c or cpp sources.
        print("no files")
        return None

    # http://cppcheck.sourceforge.net/manual.pdf
    params = ['cppcheck', '-j', str(multiprocessing.cpu_count()),
              '--enable=all', '--file-list=-']

    cppcheck_process = subprocess.Popen(
        params,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE)
    (stdout, stderr) = cppcheck_process.communicate(
        input='\n'.join(cppcheck_files).encode('utf-8'))

    if cppcheck_process.wait():
        raise Exception('Cppcheck failed')
    print(stdout.decode('utf-8'))
    print(stderr.decode('utf-8'))


def is_valgrind_safe():
    """
    Returns whether it is safe to run valgrind on our platform
    """
    src = 'unit-test-vg.c'
    exe = './unit-test-vg'
    with open(src, 'w') as h:
        h.write('#include <errno.h>\n')
        h.write('#include <stdio.h>\n')
        h.write('#include <stdlib.h>\n')
        h.write('#include <string.h>\n')
        h.write('int main() {\n')
        h.write('char *heap_str = malloc(16);\n')
        h.write('strcpy(heap_str, "RandString");\n')
        h.write('int res = strcmp("RandString", heap_str);\n')
        h.write('free(heap_str);\n')
        h.write('char errstr[64];\n')
        h.write('strerror_r(EINVAL, errstr, sizeof(errstr));\n')
        h.write('printf("%s\\n", errstr);\n')
        h.write('return res;\n')
        h.write('}\n')
    try:
        with open(os.devnull, 'w') as devnull:
            check_call(['gcc', '-O2', '-o', exe, src],
                       stdout=devnull, stderr=devnull)
            check_call(['valgrind', '--error-exitcode=99', exe],
                       stdout=devnull, stderr=devnull)
        return True
    except:
        sys.stderr.write("###### Platform is not valgrind safe ######\n")
        return False
    finally:
        os.remove(src)
        os.remove(exe)


def is_sanitize_safe():
    """
    Returns whether it is safe to run sanitizers on our platform
    """
    src = 'unit-test-sanitize.c'
    exe = './unit-test-sanitize'
    with open(src, 'w') as h:
        h.write('int main() { return 0; }\n')
    try:
        with open(os.devnull, 'w') as devnull:
            check_call(['gcc', '-O2', '-fsanitize=address',
                        '-fsanitize=undefined', '-o', exe, src],
                       stdout=devnull, stderr=devnull)
            check_call([exe], stdout=devnull, stderr=devnull)
        return True
    except:
        sys.stderr.write("###### Platform is not sanitize safe ######\n")
        return False
    finally:
        os.remove(src)
        os.remove(exe)


def maybe_make_valgrind():
    """
    Potentially runs the unit tests through valgrind for the package
    via `make check-valgrind`. If the package does not have valgrind testing
    then it just skips over this.
    """
    # Valgrind testing is currently broken by an aggressive strcmp optimization
    # that is inlined into optimized code for POWER by gcc 7+. Until we find
    # a workaround, just don't run valgrind tests on POWER.
    # https://github.com/openbmc/openbmc/issues/3315
    if not is_valgrind_safe():
        sys.stderr.write("###### Skipping valgrind ######\n")
        return
    if not make_target_exists('check-valgrind'):
        return

    try:
        cmd = make_parallel + ['check-valgrind']
        check_call_cmd(*cmd)
    except CalledProcessError:
        for root, _, files in os.walk(os.getcwd()):
            for f in files:
                if re.search('test-suite-[a-z]+.log', f) is None:
                    continue
                check_call_cmd('cat', os.path.join(root, f))
        raise Exception('Valgrind tests failed')


def maybe_make_coverage():
    """
    Potentially runs the unit tests through code coverage for the package
    via `make check-code-coverage`. If the package does not have code coverage
    testing then it just skips over this.
    """
    if not make_target_exists('check-code-coverage'):
        return

    # Actually run code coverage
    try:
        cmd = make_parallel + ['check-code-coverage']
        check_call_cmd(*cmd)
    except CalledProcessError:
        raise Exception('Code coverage failed')


class BuildSystem(object):
    """
    Build systems generally provide the means to configure, build, install and
    test software. The BuildSystem class defines a set of interfaces on top of
    which Autotools, Meson, CMake and possibly other build system drivers can
    be implemented, separating out the phases to control whether a package
    should merely be installed or also tested and analyzed.
    """
    def __init__(self, package, path):
        """Initialise the driver with properties independent of the build system

        Keyword arguments:
        package: The name of the package. Derived from the path if None
        path: The path to the package. Set to the working directory if None
        """
        self.path = "." if not path else path
        realpath = os.path.realpath(self.path)
        self.package = package if package else os.path.basename(realpath)
        self.build_for_testing = False

    def probe(self):
        """Test if the build system driver can be applied to the package

        Return True if the driver can drive the package's build system,
        otherwise False.

        Generally probe() is implemented by testing for the presence of the
        build system's configuration file(s).
        """
        raise NotImplemented

    def dependencies(self):
        """Provide the package's dependencies

        Returns a list of dependencies. If no dependencies are required then an
        empty list must be returned.

        Generally dependencies() is implemented by analysing and extracting the
        data from the build system configuration.
        """
        raise NotImplemented

    def configure(self, build_for_testing):
        """Configure the source ready for building

        Should raise an exception if configuration failed.

        Keyword arguments:
        build_for_testing: Mark the package as being built for testing rather
                           than for installation as a dependency for the
                           package under test. Setting to True generally
                           implies that the package will be configured to build
                           with debug information, at a low level of
                           optimisation and possibly with sanitizers enabled.

        Generally configure() is implemented by invoking the build system
        tooling to generate Makefiles or equivalent.
        """
        raise NotImplemented

    def build(self):
        """Build the software ready for installation and/or testing

        Should raise an exception if the build fails

        Generally build() is implemented by invoking `make` or `ninja`.
        """
        raise NotImplemented

    def install(self):
        """Install the software ready for use

        Should raise an exception if installation fails

        Like build(), install() is generally implemented by invoking `make` or
        `ninja`.
        """
        raise NotImplemented

    def test(self):
        """Build and run the test suite associated with the package

        Should raise an exception if the build or testing fails.

        Like install(), test() is generally implemented by invoking `make` or
        `ninja`.
        """
        raise NotImplemented

    def analyze(self):
        """Run any supported analysis tools over the codebase

        Should raise an exception if analysis fails.

        Some analysis tools such as scan-build need injection into the build
        system. analyze() provides the necessary hook to implement such
        behaviour. Analyzers independent of the build system can also be
        specified here but at the cost of possible duplication of code between
        the build system driver implementations.
        """
        raise NotImplemented


class Autotools(BuildSystem):
    def __init__(self, package=None, path=None):
        super(Autotools, self).__init__(package, path)

    def probe(self):
        return os.path.isfile(os.path.join(self.path, 'configure.ac'))

    def dependencies(self):
        configure_ac = os.path.join(self.path, 'configure.ac')

        contents = ''
        # Prepend some special function overrides so we can parse out
        # dependencies
        for macro in DEPENDENCIES.keys():
            contents += ('m4_define([' + macro + '], [' + macro + '_START$' +
                         str(DEPENDENCIES_OFFSET[macro] + 1) +
                         macro + '_END])\n')
        with open(configure_ac, "rt") as f:
            contents += f.read()

        autoconf_cmdline = ['autoconf', '-Wno-undefined', '-']
        autoconf_process = subprocess.Popen(autoconf_cmdline,
                                            stdin=subprocess.PIPE,
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE)
        document = contents.encode('utf-8')
        (stdout, stderr) = autoconf_process.communicate(input=document)
        if not stdout:
            print(stderr)
            raise Exception("Failed to run autoconf for parsing dependencies")

        # Parse out all of the dependency text
        matches = []
        for macro in DEPENDENCIES.keys():
            pattern = '(' + macro + ')_START(.*?)' + macro + '_END'
            for match in re.compile(pattern).finditer(stdout.decode('utf-8')):
                matches.append((match.group(1), match.group(2)))

        # Look up dependencies from the text
        found_deps = []
        for macro, deptext in matches:
            for potential_dep in deptext.split(' '):
                for known_dep in DEPENDENCIES[macro].keys():
                    if potential_dep.startswith(known_dep):
                        found_deps.append(DEPENDENCIES[macro][known_dep])

        return found_deps

    def _configure_feature(self, flag, enabled):
        """
        Returns an configure flag as a string

        Parameters:
        flag                The name of the flag
        enabled             Whether the flag is enabled or disabled
        """
        return '--' + ('enable' if enabled else 'disable') + '-' + flag

    def configure(self, build_for_testing):
        self.build_for_testing = build_for_testing
        conf_flags = [
            self._configure_feature('silent-rules', False),
            self._configure_feature('examples', build_for_testing),
            self._configure_feature('tests', build_for_testing),
        ]
        if not TEST_ONLY:
            conf_flags.extend([
                self._configure_feature('code-coverage', build_for_testing),
                self._configure_feature('valgrind', build_for_testing),
            ])
        # Add any necessary configure flags for package
        if CONFIGURE_FLAGS.get(self.package) is not None:
            conf_flags.extend(CONFIGURE_FLAGS.get(self.package))
        for bootstrap in ['bootstrap.sh', 'bootstrap', 'autogen.sh']:
            if os.path.exists(bootstrap):
                check_call_cmd('./' + bootstrap)
                break
        check_call_cmd('./configure', *conf_flags)

    def build(self):
        check_call_cmd(*make_parallel)

    def install(self):
        check_call_cmd('sudo', '-n', '--', *(make_parallel + ['install']))

    def test(self):
        try:
            cmd = make_parallel + ['check']
            for i in range(0, args.repeat):
                check_call_cmd(*cmd)
        except CalledProcessError:
            for root, _, files in os.walk(os.getcwd()):
                if 'test-suite.log' not in files:
                    continue
                check_call_cmd('cat', os.path.join(root, 'test-suite.log'))
            raise Exception('Unit tests failed')

    def analyze(self):
        maybe_make_valgrind()
        maybe_make_coverage()
        run_cppcheck()


class CMake(BuildSystem):
    def __init__(self, package=None, path=None):
        super(CMake, self).__init__(package, path)

    def probe(self):
        return os.path.isfile(os.path.join(self.path, 'CMakeLists.txt'))

    def dependencies(self):
        return []

    def configure(self, build_for_testing):
        self.build_for_testing = build_for_testing
        check_call_cmd('cmake', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '.')

    def build(self):
        check_call_cmd('cmake', '--build', '.', '--', '-j',
                       str(multiprocessing.cpu_count()))

    def install(self):
        pass

    def test(self):
        if make_target_exists('test'):
            check_call_cmd('ctest', '.')

    def analyze(self):
        if TEST_ONLY:
            return

        if os.path.isfile('.clang-tidy'):
            check_call_cmd('run-clang-tidy-10.py', '-p', '.')
        maybe_make_valgrind()
        maybe_make_coverage()
        run_cppcheck()


class Meson(BuildSystem):
    def __init__(self, package=None, path=None):
        super(Meson, self).__init__(package, path)

    def probe(self):
        return os.path.isfile(os.path.join(self.path, 'meson.build'))

    def dependencies(self):
        meson_build = os.path.join(self.path, 'meson.build')
        if not os.path.exists(meson_build):
            return []

        found_deps = []
        for root, dirs, files in os.walk(self.path):
            if 'meson.build' not in files:
                continue
            with open(os.path.join(root, 'meson.build'), 'rt') as f:
                build_contents = f.read()
            pattern = r"dependency\('([^']*)'.*?\),?\n"
            for match in re.finditer(pattern, build_contents):
                group = match.group(1)
                maybe_dep = DEPENDENCIES['PKG_CHECK_MODULES'].get(group)
                if maybe_dep is not None:
                    found_deps.append(maybe_dep)

        return found_deps

    def _parse_options(self, options_file):
        """
        Returns a set of options defined in the provides meson_options.txt file

        Parameters:
        options_file        The file containing options
        """
        oi = optinterpreter.OptionInterpreter('')
        oi.process(options_file)
        return oi.options

    def _configure_boolean(self, val):
        """
        Returns the meson flag which signifies the value

        True is true which requires the boolean.
        False is false which disables the boolean.

        Parameters:
        val                 The value being converted
        """
        if val is True:
            return 'true'
        elif val is False:
            return 'false'
        else:
            raise Exception("Bad meson boolean value")

    def _configure_feature(self, val):
        """
        Returns the meson flag which signifies the value

        True is enabled which requires the feature.
        False is disabled which disables the feature.
        None is auto which autodetects the feature.

        Parameters:
        val                 The value being converted
        """
        if val is True:
            return "enabled"
        elif val is False:
            return "disabled"
        elif val is None:
            return "auto"
        else:
            raise Exception("Bad meson feature value")

    def _configure_option(self, opts, key, val):
        """
        Returns the meson flag which signifies the value
        based on the type of the opt

        Parameters:
        opt                 The meson option which we are setting
        val                 The value being converted
        """
        if isinstance(opts[key], coredata.UserBooleanOption):
            str_val = self._configure_boolean(val)
        elif isinstance(opts[key], coredata.UserFeatureOption):
            str_val = self._configure_feature(val)
        else:
            raise Exception('Unknown meson option type')
        return "-D{}={}".format(key, str_val)

    def configure(self, build_for_testing):
        self.build_for_testing = build_for_testing
        meson_options = {}
        if os.path.exists("meson_options.txt"):
            meson_options = self._parse_options("meson_options.txt")
        meson_flags = [
            '-Db_colorout=never',
            '-Dwerror=true',
            '-Dwarning_level=3',
        ]
        if build_for_testing:
            meson_flags.append('--buildtype=debug')
        else:
            meson_flags.append('--buildtype=debugoptimized')
        if 'tests' in meson_options:
            meson_flags.append(self._configure_option(meson_options, 'tests', build_for_testing))
        if 'examples' in meson_options:
            meson_flags.append(self._configure_option(meson_options, 'examples', build_for_testing))
        if MESON_FLAGS.get(self.package) is not None:
            meson_flags.extend(MESON_FLAGS.get(self.package))
        try:
            check_call_cmd('meson', 'setup', '--reconfigure', 'build',
                           *meson_flags)
        except:
            shutil.rmtree('build')
            check_call_cmd('meson', 'setup', 'build', *meson_flags)

    def build(self):
        check_call_cmd('ninja', '-C', 'build')

    def install(self):
        check_call_cmd('sudo', '-n', '--', 'ninja', '-C', 'build', 'install')

    def test(self):
        try:
            check_call_cmd('meson', 'test', '-C', 'build')
        except CalledProcessError:
            for root, _, files in os.walk(os.getcwd()):
                if 'testlog.txt' not in files:
                    continue
                check_call_cmd('cat', os.path.join(root, 'testlog.txt'))
            raise Exception('Unit tests failed')

    def _setup_exists(self, setup):
        """
        Returns whether the meson build supports the named test setup.

        Parameter descriptions:
        setup              The setup target to check
        """
        try:
            with open(os.devnull, 'w') as devnull:
                output = subprocess.check_output(
                        ['meson', 'test', '-C', 'build',
                         '--setup', setup, '-t', '0'],
                        stderr=subprocess.STDOUT)
        except CalledProcessError as e:
            output = e.output
        output = output.decode('utf-8')
        return not re.search('Test setup .* not found from project', output)

    def _maybe_valgrind(self):
        """
        Potentially runs the unit tests through valgrind for the package
        via `meson test`. The package can specify custom valgrind
        configurations by utilizing add_test_setup() in a meson.build
        """
        if not is_valgrind_safe():
            sys.stderr.write("###### Skipping valgrind ######\n")
            return
        try:
            if self._setup_exists('valgrind'):
                check_call_cmd('meson', 'test', '-C', 'build',
                               '--setup', 'valgrind')
            else:
                check_call_cmd('meson', 'test', '-C', 'build',
                               '--wrapper', 'valgrind')
        except CalledProcessError:
            for root, _, files in os.walk(os.getcwd()):
                if 'testlog-valgrind.txt' not in files:
                    continue
                cat_args = os.path.join(root, 'testlog-valgrind.txt')
                check_call_cmd('cat', cat_args)
            raise Exception('Valgrind tests failed')

    def analyze(self):
        if TEST_ONLY:
            return

        self._maybe_valgrind()

        # Run clang-tidy only if the project has a configuration
        if os.path.isfile('.clang-tidy'):
            check_call_cmd('run-clang-tidy-10.py', '-p',
                           'build')
        # Run the basic clang static analyzer otherwise
        else:
            check_call_cmd('ninja', '-C', 'build',
                           'scan-build')

        # Run tests through sanitizers
        # b_lundef is needed if clang++ is CXX since it resolves the
        # asan symbols at runtime only. We don't want to set it earlier
        # in the build process to ensure we don't have undefined
        # runtime code.
        if is_sanitize_safe():
            check_call_cmd('meson', 'configure', 'build',
                           '-Db_sanitize=address,undefined',
                           '-Db_lundef=false')
            check_call_cmd('meson', 'test', '-C', 'build',
                           '--logbase', 'testlog-ubasan')
            # TODO: Fix memory sanitizer
            # check_call_cmd('meson', 'configure', 'build',
            #                '-Db_sanitize=memory')
            # check_call_cmd('meson', 'test', '-C', 'build'
            #                '--logbase', 'testlog-msan')
            check_call_cmd('meson', 'configure', 'build',
                           '-Db_sanitize=none', '-Db_lundef=true')
        else:
            sys.stderr.write("###### Skipping sanitizers ######\n")

        # Run coverage checks
        check_call_cmd('meson', 'configure', 'build',
                       '-Db_coverage=true')
        self.test()
        # Only build coverage HTML if coverage files were produced
        for root, dirs, files in os.walk('build'):
            if any([f.endswith('.gcda') for f in files]):
                check_call_cmd('ninja', '-C', 'build',
                               'coverage-html')
                break
        check_call_cmd('meson', 'configure', 'build',
                       '-Db_coverage=false')
        run_cppcheck()


class Package(object):
    def __init__(self, name=None, path=None):
        self.supported = [Meson, Autotools, CMake]
        self.name = name
        self.path = path
        self.test_only = False

    def build_systems(self):
        instances = (system(self.name, self.path) for system in self.supported)
        return (instance for instance in instances if instance.probe())

    def build_system(self, preferred=None):
        systems = list(self.build_systems())

        if not systems:
            return None

        if preferred:
            return {type(system): system for system in systems}[preferred]

        return next(iter(systems))

    def install(self, system=None):
        if not system:
            system = self.build_system()

        system.configure(False)
        system.build()
        system.install()

    def _test_one(self, system):
        system.configure(True)
        system.build()
        system.install()
        system.test()
        system.analyze()

    def test(self):
        for system in self.build_systems():
            self._test_one(system)


def find_file(filename, basedir):
    """
    Finds all occurrences of a file in the base directory
    and passes them back with their relative paths.

    Parameter descriptions:
    filename              The name of the file to find
    basedir               The base directory search in
    """

    filepaths = []
    for root, dirs, files in os.walk(basedir):
        if filename in files:
            filepaths.append(os.path.join(root, filename))
    return filepaths


if __name__ == '__main__':
    # CONFIGURE_FLAGS = [GIT REPO]:[CONFIGURE FLAGS]
    CONFIGURE_FLAGS = {
        'phosphor-logging':
        ['--enable-metadata-processing', '--enable-openpower-pel-extension',
         'YAML_DIR=/usr/local/share/phosphor-dbus-yaml/yaml']
    }

    # MESON_FLAGS = [GIT REPO]:[MESON FLAGS]
    MESON_FLAGS = {
    }

    # DEPENDENCIES = [MACRO]:[library/header]:[GIT REPO]
    DEPENDENCIES = {
        'AC_CHECK_LIB': {'mapper': 'phosphor-objmgr'},
        'AC_CHECK_HEADER': {
            'host-ipmid': 'phosphor-host-ipmid',
            'blobs-ipmid': 'phosphor-ipmi-blobs',
            'sdbusplus': 'sdbusplus',
            'sdeventplus': 'sdeventplus',
            'stdplus': 'stdplus',
            'gpioplus': 'gpioplus',
            'phosphor-logging/log.hpp': 'phosphor-logging',
        },
        'AC_PATH_PROG': {'sdbus++': 'sdbusplus'},
        'PKG_CHECK_MODULES': {
            'phosphor-dbus-interfaces': 'phosphor-dbus-interfaces',
            'libipmid': 'phosphor-host-ipmid',
            'libipmid-host': 'phosphor-host-ipmid',
            'sdbusplus': 'sdbusplus',
            'sdeventplus': 'sdeventplus',
            'stdplus': 'stdplus',
            'gpioplus': 'gpioplus',
            'phosphor-logging': 'phosphor-logging',
            'phosphor-snmp': 'phosphor-snmp',
            'ipmiblob': 'ipmi-blob-tool',
            'hei': 'openpower-libhei',
        },
    }

    # Offset into array of macro parameters MACRO(0, 1, ...N)
    DEPENDENCIES_OFFSET = {
        'AC_CHECK_LIB': 0,
        'AC_CHECK_HEADER': 0,
        'AC_PATH_PROG': 1,
        'PKG_CHECK_MODULES': 1,
    }

    # DEPENDENCIES_REGEX = [GIT REPO]:[REGEX STRING]
    DEPENDENCIES_REGEX = {
        'phosphor-logging': r'\S+-dbus-interfaces$'
    }

    # Set command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("-w", "--workspace", dest="WORKSPACE", required=True,
                        help="Workspace directory location(i.e. /home)")
    parser.add_argument("-p", "--package", dest="PACKAGE", required=True,
                        help="OpenBMC package to be unit tested")
    parser.add_argument("-t", "--test-only", dest="TEST_ONLY",
                        action="store_true", required=False, default=False,
                        help="Only run test cases, no other validation")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print additional package status messages")
    parser.add_argument("-r", "--repeat", help="Repeat tests N times",
                        type=int, default=1)
    parser.add_argument("-b", "--branch", dest="BRANCH", required=False,
                        help="Branch to target for dependent repositories",
                        default="master")
    parser.add_argument("-n", "--noformat", dest="FORMAT",
                        action="store_false", required=False,
                        help="Whether or not to run format code")
    args = parser.parse_args(sys.argv[1:])
    WORKSPACE = args.WORKSPACE
    UNIT_TEST_PKG = args.PACKAGE
    TEST_ONLY = args.TEST_ONLY
    BRANCH = args.BRANCH
    FORMAT_CODE = args.FORMAT
    if args.verbose:
        def printline(*line):
            for arg in line:
                print(arg, end=' ')
            print()
    else:
        def printline(*line):
            pass

    CODE_SCAN_DIR = WORKSPACE + "/" + UNIT_TEST_PKG

    # First validate code formatting if repo has style formatting files.
    # The format-code.sh checks for these files.
    if FORMAT_CODE:
        check_call_cmd("./format-code.sh", CODE_SCAN_DIR)

    # Check if this repo has a supported make infrastructure
    pkg = Package(UNIT_TEST_PKG, os.path.join(WORKSPACE, UNIT_TEST_PKG))
    if not pkg.build_system():
        print("No valid build system, exit")
        sys.exit(0)

    prev_umask = os.umask(000)

    # Determine dependencies and add them
    dep_added = dict()
    dep_added[UNIT_TEST_PKG] = False

    # Create dependency tree
    dep_tree = DepTree(UNIT_TEST_PKG)
    build_dep_tree(UNIT_TEST_PKG,
                   os.path.join(WORKSPACE, UNIT_TEST_PKG),
                   dep_added,
                   dep_tree,
                   BRANCH)

    # Reorder Dependency Tree
    for pkg_name, regex_str in DEPENDENCIES_REGEX.items():
        dep_tree.ReorderDeps(pkg_name, regex_str)
    if args.verbose:
        dep_tree.PrintTree()

    install_list = dep_tree.GetInstallList()

    # We don't want to treat our package as a dependency
    install_list.remove(UNIT_TEST_PKG)

    # Install reordered dependencies
    for dep in install_list:
        build_and_install(dep, False)

    # Run package unit tests
    build_and_install(UNIT_TEST_PKG, True)

    os.umask(prev_umask)

    # Run any custom CI scripts the repo has, of which there can be
    # multiple of and anywhere in the repository.
    ci_scripts = find_file('run-ci.sh', os.path.join(WORKSPACE, UNIT_TEST_PKG))
    if ci_scripts:
        os.chdir(os.path.join(WORKSPACE, UNIT_TEST_PKG))
        for ci_script in ci_scripts:
            check_call_cmd('sh', ci_script)
