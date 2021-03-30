#Makefile for openbmctool
# Copyright (c) 2017 International Business Machines.
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#        http://www.apache.org/licenses/LICENSE-2.0
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.

VER=1.18
REL=1
ARCH=noarch
PROD=openbmctool
RHEL_VER=7
NAME=$(PROD)-$(VER)-$(REL).$(ARCH)

# Need to test RPMDIR to see if it is set. Otherwise set it.
RPMDIR := $(if $(RPMDIR),$(RPMDIR),$(shell pwd)/build/rpm)

# Need to test DEBDIR to see if it is set. Otherwise set it.
DEBDIR := $(if $(DEBDIR),$(DEBDIR),$(shell pwd)/build/deb)

default: 
	mkdir -p ./build
clean: ;rm -rf ./build
remove: 
	rm -f /usr/bin/openbmctool
	rm -f /opt/ibm/ras/bin/openbmctool.py
	rm -f /opt/ibm/ras/lib/policyTable.json
install: 
	cp ../openbmctool.py $(DESTDIR)/opt/ibm/ras/bin
	cp ../policyTable.json $(DESTDIR)/opt/ibm/ras/lib
	ln -s -f $(DESTDIR)/opt/ibm/ras/bin/openbmctool.py $(DESTDIR)/usr/bin/openbmctool

rpm: default
	rm -rf $(RPMDIR)
	mkdir -p $(RPMDIR)
	mkdir -p $(RPMDIR)/openbmctool
	for i in BUILD BUILDROOT RPMS SOURCES SPECS SRPMS; do mkdir -p $(RPMDIR)/$$i; done
	cp openbmctool-rhel$(RHEL_VER).spec $(RPMDIR)/SPECS
	cp ../openbmctool.py ../policyTable.json $(RPMDIR)/openbmctool/
	tar -cvzf $(RPMDIR)/SOURCES/openbmctool-$(VER)-$(REL).tgz -C $(RPMDIR) openbmctool
	rpmbuild --define '_topdir $(RPMDIR)' --define "_version $(VER)" --define "_release $(REL)" -bb $(RPMDIR)/SPECS/openbmctool-rhel$(RHEL_VER).spec

deb: default
	rm -rf $(DEBDIR)
	mkdir -p $(DEBDIR)
	for i in bin lib; do mkdir -p $(DEBDIR)/opt/ibm/ras/$$i; done
	mkdir -p $(DEBDIR)/usr/bin/
	make install DESTDIR=$(DEBDIR)
	mkdir -p $(DEBDIR)/DEBIAN
	cp control $(DEBDIR)/DEBIAN
	cd $(DEBDIR); find opt -type f -exec md5sum "{}" + >> DEBIAN/md5sums
	cd $(DEBDIR); find usr -type f -exec md5sum "{}" + >> DEBIAN/md5sums
	cd $(DEBDIR); echo "#!/bin/bash" >> DEBIAN/postinst
	cd $(DEBDIR); echo "ln -s -f /opt/ibm/ras/bin/openbmctool.py /usr/bin/openbmctool" >> DEBIAN/postinst
	chmod +x $(DEBDIR)/DEBIAN/postinst
	chmod +x $(DEBDIR)/opt/ibm/ras/bin/openbmctool.py
	dpkg-deb -b $(DEBDIR) $(DEBDIR)/DEBIAN/openbmctool-$(VER).$(REL)-$(ARCH).deb

