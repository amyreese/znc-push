version := $(shell git describe --dirty)
curl=no

ifneq ($(curl),no)
	flags=-DUSE_CURL -lcurl
else
	flags=
endif

push.so: push.cpp
	sed -i -e "s|PUSHVERSION \".*\"|PUSHVERSION \"$(version)\"|" push.cpp
	CXXFLAGS="$(CXXFLAGS) $(flags)" znc-buildmod push.cpp
	sed -i -e "s|PUSHVERSION \".*\"|PUSHVERSION \"dev\"|" push.cpp

install: push.so
	cp push.so $(HOME)/.znc/modules/push.so

clean:
	-rm -f push.so
