version := $(shell git describe --dirty)

push.pyc: push.py
	cp push.py push.copy.py

	sed -i -e "s|VERSION = '.*'|VERSION = '$(version)'|" push.copy.py
	python3 -m compileall push.copy.py
	cp __pycache__/push.*.pyc push.pyc

	rm push.copy.py

lint: push.py
	flake8 push.py

install: push.pyc
	cp push.pyc $(HOME)/.znc/modules/push.pyc

clean:
	-rm -rf push.so push.pyc __pycache__
