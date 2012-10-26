push.so: push.cpp
	znc-buildmod push.cpp

install: push.so
	cp push.so $(HOME)/.znc/modules/push.so

clean:
	-rm -f push.so
