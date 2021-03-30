# Copyright (c) 2017 International Business Machines.  All right reserved.
%define _binaries_in_noarch_packages_terminate_build   0
Summary: IBM OpenBMC tool
Name: openbmctool
Version: %{_version}
Release: %{_release}
License: Apache 2.0
Group: System Environment/Base
BuildArch: noarch
URL: http://www.ibm.com/
Source0: %{name}-%{version}-%{release}.tgz
Prefix: /opt
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

Requires: python36
Requires: python36-requests
Requires: python36-simplejson

# Turn off the brp-python-bytecompile script
%global __os_install_post %(echo '%{__os_install_post}' | sed -e 's!/usr/lib[^[:space:]]*/brp-python-bytecompile[[:space:]].*$!!g')

%description
This package is to be applied to any linux machine that will be used to manage or interact with the IBM OpenBMC.
It provides key functionality to easily work with the IBM OpenBMC RESTful API, making BMC management easy.

%prep
%setup -q -n %{name}

%install
export DESTDIR=$RPM_BUILD_ROOT/opt/ibm/ras
mkdir -p $DESTDIR/bin
mkdir -p $DESTDIR/lib
cp openbmctool*.py $DESTDIR/bin
cp *.json $DESTDIR/lib


%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(775,root,root) /opt/ibm/ras/bin/openbmctool.py
%attr(664,root,root)/opt/ibm/ras/lib/policyTable.json

%post
ln -s -f /opt/ibm/ras/bin/openbmctool.py /usr/bin/openbmctool
%changelog
