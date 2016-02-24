version = $(shell git describe --dirty || echo dev)
curl=no
command=no

flags=

ifneq ($(curl),no)
	flags+=-DUSE_CURL -lcurl
endif

ifneq ($(command),no)
	flags+=-DUSE_COMMAND
endif

all: push.so

push.so: push.cpp
	CXXFLAGS="$(CXXFLAGS) -DPUSHVERSION=\"$(version)\" $(flags)" LIBS="$(LIBS) $(flags)" \
		 znc-buildmod push.cpp

install: push.so
	mkdir -p $(HOME)/.znc/modules/
	cp push.so $(HOME)/.znc/modules/push.so

clean:
	-rm -f push.so
