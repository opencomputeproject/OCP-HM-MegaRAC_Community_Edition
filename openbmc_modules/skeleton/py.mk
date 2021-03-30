PYTHON?=python
all:
	$(PYTHON) setup.py build

clean:
	rm -rf build

install: all
	$(PYTHON) setup.py install --root=$(DESTDIR) --prefix=$(PREFIX)
