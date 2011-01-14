ZNC to Notifo
=============

ZNC to Notifo is a module for [ZNC][] that will send notifications to a [Notifo][] account
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
you're done!


Commands
--------

*   `set <option> <value>`

    Allows you to modify configuration values.

*   `get [<option>]`

    Allows you to see current configuration values.

*   `status`

    Check the status of current conditions.

*   `send <message>`

    Manually trigger a notification with the given message.  Useful for testing to validate
    credentials, etc.


Configuration
-------------

### Conditions

*   `client_count_less_than = 0`

    Notifications will only be sent if the number of connected IRC clients is less than this
    value.  A value of 0 (zero) will disable this condition.


### Notifications

*   `message_length = 100`

    Maximum length of the notification message to be sent.  The message will be nicely
    truncated and ellipsized at or before this length is reached.  A value of 0 (zero) will
    disable this option.

*   `message_url = ""`

    URI that will be sent with the notification to Notifo.  This could be a web address or a
    local scheme to access a mobile application.


License
-------

This project is licensed under the MIT license.  See the `LICENSE` file for details.



[Notifo]: http://notifo.com "Notifo, Mobile Notifications for Everything"
[ZNC]: http://en.znc.in "ZNC, an advanced IRC bouncer"

# vim:set ft= expandtab tabstop=4 shiftwidth=4:
