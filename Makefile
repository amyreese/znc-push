version := $(shell git describe --dirty)

push.pyc: push.py
	cp push.py push.py.orig

	sed -i -e "s|VERSION = '.*'|VERSION = '$(version)'|" push.py
	python3 -m compileall push.py
	cp __pycache__/push.*.pyc push.pyc

	mv push.py.orig push.py

install: push.pyc
	cp push.pyc $(HOME)/.znc/modules/push.pyc

clean:
	-rm -rf push.pyc __pycache__
