ZNC to Notifo
=============

ZNC to Notifo is a module for [znc][] that will send notifications to a [notifo][] account
for any private message or channel highlight that matches a configurable set of conditions.


Compiling
---------

If you have `make` installed, you can compile the module with:

    $ make

Otherwise, run the full command:

    $ znc-build notifo.cpp


Installation
------------

Copy the compiled module into your ZNC profile:

    $ cp notifo.so ~/.znc/modules/

Now, load the module in ZNC:

    /msg *status loadmod notifo

Then set your Notifo username and API secret:

    /msg *notifo set username foo
	/msg *notifo set secret ...

At this point, it should start sending notifications every time you get a private message
or someone says your name in a channel.  If this is everything you wanted, congratulations,
you're done!  If you want to tweak the notfication conditions, continue on to configuration.


Configuration
-------------

There is none yet.... :'(


License
-------

This project is licensed under the MIT license.  See the `LICENSE` file for details.
