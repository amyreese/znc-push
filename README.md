ZNC to Notifo
=============

ZNC to Notifo is a module for [ZNC][] that will send notifications to a [Notifo][] account
for any private message or channel highlight that matches a configurable set of conditions.

This project is still a Work In Progress, but should be functional enough and stable enough
for everyday usage.  Users are more than welcome to submit feature requests or patches for
discussion or inclusion.  Bug reports and feature requests can be submitted to
[my bug tracker][mantis] or sent via email.

ZNC to Notifo was created by [John Reese](http://johnmreese.com) and designed to fill a
personal need.  It may not fit your use cases, but any and all feedback would be greatly
appreciated.


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

*   `help`

    Links you to this fine document.

*   `set <option> <value>`

    Allows you to modify configuration values.

*   `get [<option>]`

    Allows you to see current configuration values.

*   `unset <option>`

    Allows you to reset a configuration option back to the default value.

*   `status`

    Check the status of current conditions.

*   `send <message>`

    Manually trigger a notification with the given message.  Useful for testing to validate
    credentials, etc.


Configuration
-------------

### Conditions

*   `away_only = "no"`

    If set to "yes", notifications will only be sent if the user has set their `/away` status.

*   `client_count_less_than = 0`

    Notifications will only be sent if the number of connected IRC clients is less than this
    value.  A value of 0 (zero) will disable this condition.

*   `last_notification = 300`

    Time in seconds since the last notification sent from that channel or query window.
    Notifications will only be sent if the elapsed time is greater than this value.  A value
    of 0 (zero) will disable this condition.

    Note that this condition keeps track of the last notification sent from each channel and
    query window separately, so a recent PM from Joe will not affect a notification sent
    from channel #foo.


### Notifications

*   `message_length = 100`

    Maximum length of the notification message to be sent.  The message will be nicely
    truncated and ellipsized at or before this length is reached.  A value of 0 (zero) will
    disable this option.

*   `message_url = ""`

    URI that will be sent with the notification to Notifo.  This could be a web address or a
    local scheme to access a mobile application.


Roadmap
-------

### Conditions

*   User inactivity: How long, in seconds, since the last action made by user, in any
    channel or query window.

*   Channel inactivity: How long, in seconds, since the last action made by the user in
    the same channel or query window.

*   Highlights: Strings to trigger a channel notification, in addition to the default
    highlight when your nick is mentioned.

*   Nick blacklist: List of nicks to never send notifications from, e.g. channel bots.

### Settings

*   Customizable notification titles and message formats.


License
-------

This project is licensed under the MIT license.  See the `LICENSE` file for details.



[mantis]: http://leetcode.net/mantis
[Notifo]: http://notifo.com "Notifo, Mobile Notifications for Everything"
[ZNC]: http://en.znc.in "ZNC, an advanced IRC bouncer"

<!-- vim:set ft= expandtab tabstop=4 shiftwidth=4: -->
