version := $(shell git describe)

push.so: push.cpp
	sed -i -e "s|PUSHVERSION \".*\"|PUSHVERSION \"$(version)\"|" push.cpp
	znc-buildmod push.cpp
	sed -i -e "s|PUSHVERSION \".*\"|PUSHVERSION \"dev\"|" push.cpp

install: push.so
	cp push.so $(HOME)/.znc/modules/push.so

clean:
	-rm -f push.so
