ZNC Push
========

[FAQ][] | [Example Configuration][examples]


Overview
--------

ZNC Push is a module for [ZNC][] that will send notifications to multiple push
notification services for any private message or channel highlight that matches
a configurable set of conditions.

ZNC Push current supports the following services:

* [Boxcar][]
* [Notify My Android][] (NMA)
* [Pushover][]
* [Prowl][]
* [Supertoasty][]
* [PushBullet][]
* [Airgram][]
* [Faast][]
* Custom URL GET and POST requests

This project is still a work in progress, but should be functional enough and
stable enough for everyday usage.  Users are more than welcome to submit
feature requests or patches for discussion or inclusion.  Bug reports and
feature requests can be submitted to [the repository issues list][issues], or
sent via email.

ZNC Push currently supports ZNC versions 1.0 and newer. To use ZNC Push with
older versions of ZNC, the "legacy" branch is available, but unsupported and
unmaintained.

ZNC Push was created by [John Reese](http://johnmreese.com) and designed to
fill a personal need.  It may not fit your use cases, but any and all feedback
would be greatly appreciated.


Dependencies
------------

ZNC Push is now a Python module and, as such, requires ZNC to be compiled with
support for [modpython][].  If you are building ZNC from source (which you
should do), you need to configure ZNC with the `--enable-python` flag.

ZNC Push also requires the Python [requests][] module, which can be installed
on Ubuntu via:

    $ sudo apt-get install python3-requests

The [requests][] module can also be installed via pip, using something like:

    $ sudo pip3 install requests


Upgrade
-------

IMPORTANT: The new ZNC Push module runs only as a user-level module, and can
potentially cause confusion and problems if you don't first unload *push from
every network in ZNC, and then remove the old module.  ZNC Push will find and
import all of your existing network configurations automatically at first run.

From every user/network you have in ZNC, you should do the following:

    /msg *status unloadmod

Then remove the old module from your znc path:

    $ rm ~/.znc/modules/push.so

After this, you can proceed to the next section.


Install
-------

If you have `make` installed, you can pre-compile the module and install it in
your `.znc` path with:

    $ make install

Otherwise, you can simply copy `push.py` into your znc path like:

    $ cp push.py ~/.znc/modules/


Setup
-----

Now, load the module in ZNC:

    /msg *status loadmod push

This will load ZNC Push as a user-level module, which means it will be active
for every network your user account has.

Then select the push service you want to use, and set your username and secret
as needed. The secret is usually not your password, and can generally be
obtained by logging into the service's website and looking in your profile
or settings.

As an example, to set up [Pushover][], send the following commands to *push:

    /msg *push set service pushover
    /msg *push set username [your user key]
	/msg *push set secret [your api key]

For some services, such as [Boxcar][], you may need to "subscribe" before push
notifications can be sent to your device.  Once you've done the above steps to
configure the service, you can use the following command to subscribe:

    /msg *push subscribe

For further, detailed instructions specific to each push notification service,
the following documentation is available:

*   [Pushover](doc/pushover.md)

IMPORTANT: By default, ZNC Push is very conservative, and will only send
notifications when its reasonably confident that you aren't paying attention
to IRC, or to that specific channel.  The intention of this behavior is to
prevent bombarding your phone with push notifications if someone starts
sending you a lot of private messages, or is continuously mentioning you in a
channel.

However, if you would prefer to receive more push notifications, please look
at the [Conditions](#conditions) section of the [Configuration](#configuration)
guide for what options control when and how you receive push notifications.
ZNC Push is highly configurable, and should be able to support almost any
method of filtering notifications that you can imagine.  If you need more
assistance with configuration, please feel free to ask for help in `#znc` on
Freenode or by email.


Commands
--------

*   `help`

    Links you to this fine document.

*   `version`

    Tells you the tagged build version currently running.

*   `set [/<network>] [<channel>] <option> <value>`

    Allows you to modify configuration values by replacing the current or
    default value.

    By specifying a network or channel, this will create an "override" value
    that takes precedence over the global value in any context that matches
    that network or channel.

*   `append [/<network>] [<channel>] <option> <value>`

    Allows you to add a string to the end of a configuration value.
    Automatically adds a space to separate the appended value from the
    existing value.

    By specifying a network or channel, this will only append the given string
    to the "override" value for that network or channel.  If an override value
    does not currently exist, then it will create an override containing just
    the given string.

*   `prepend [/<network>] [<channel>] <option> <value>`

    Allows you to add a string to the beginning of a configuration value.
    Automatically adds a space to separate the prepended value from the
    existing value.

    By specifying a network or channel, this will only prepend the given string
    to the "override" value for that network or channel.  If an override value
    does not currently exist, then it will create an override containing just
    the given string.

*   `unset [/<network>] [<channel>] <option>`

    Allows you to reset a configuration option back to the default value.

    By specifying a network or channel, this will remove any "override" value
    matching the given network or channel, and will leave the global value
    intact.  If you would like to remove the global value as well as all
    override values, use the `reset` command instead.

*   `reset <option>`

    This will remove *all* configured values and overrides for a given option,
    returning the option to a completely default value.  To only remove an
    individual override value, use the `unset` command instead.

*   `get [/<network>] [<channel>] [<option>]`

    Allows you to see current configuration values.  Options that currently
    have "override" values defined will have an asterisk (*) at the end of the
    option name; options that don't allow overrides will instead have an at
    symbol (@) at the end of the option name.

    By specifying a network or channel, this will display any "override" values
    that match the given network or channel.  Global or default values will
    only show if there are no overrides in effect for the given network or
    channel.  To see both global and override values at once

*   `dump`

    Generates a listing of all configuration values, including all network
    or channel override values.  This is the best way to see your entire
    configuration at a glance.

*   `send [/<network>] [<channel>] <message>`

    Manually trigger a push notification with the given message.  Useful for
    testing to validate credentials or configuration.

    By specifying a network or channel, this will use any configured "override"
    values for the given network or channel, rather than global values.

*   `subscribe [/<network>] [<channel>]`

    Send a subscription request for the selected service.  This is required by
    certain services, such as Boxcar, before ZNC Push can send any messages to
    your account.

    By specifying a network or channel, this will use any configured "override"
    values for the given network or channel, rather than global values.


Configuration
-------------

### Keyword Expansion

Some configuration options allow for optional keyword expansion, which happens
while preparing to send the push notification.  Expansion is performed each
time a notification is sent.  Expansion is only performed on options that are
explicitly marked as featuring expansion.

The following keywords will be replaced with the appropriate value:

*   `{context}`: the channel or query window context
*   `{nick}`: the nick that sent the message
*   `{datetime}`: [ISO 8601][] date string, in server-local time
*   `{unixtime}`: unix-style integer timestamp
*   `{title}`: the default title for the notification
*   `{message}`: the shortened message contents
*   `{username}`: the configured username string
*   `{secret}`: the configured secret string

As an example, a value of `http://domain/{context}/{datetime}` would be
expanded to something similar to `http://domain/#channel/2011-03-09 14:25:09`,
or `http://domain/{nick}/{unixtime}` to `http://domain/somenick/1299685136`.


### Push Services

*   `service` Default: ` `

    Short name for the push notification service that you want to use.
    Must be set before ZNC Push can send any notifications.

    When setting this value, ZNC Push will notify you of all other options
    that are required to be set by the chosen service.

    Possible values include:

    *   "airgram"
    *   "boxcar"
    *   "faast"
    *   "nma"
    *   "prowl"
    *   "pushbullet"
    *   "pushover"
    *   "supertoasty"
    *   "url"

*   `username` Default: ` `

    User account that should receive push notifications.

*   `secret` Default: ` `

    Authentication token for push notifications.

*   `target` Default: ` `

    Device or target name for push notifications.


### Notifications

*   `message_content` Default: `{channel} [{nick}] {message}`

    Message content that will be sent for the push notification.
    Keyword expansion is performed on this value.

*   `message_length` Default: `100`

    Maximum length of the notification message to be sent.  The message will
    be nicely truncated and ellipsized at or before this length is reached.
    A value of 0 (zero) will disable this option.

*   `message_title` Default: `{title}`

    Title that will be provided for the push notification.
    Keyword expansion is performed on this value.

*   `message_uri` Default: ` `

    URI that will be sent with the push notification.  This could be a web
    address, or a local scheme to access a mobile application.
    Keyword expansion is performed on this value.

*   `message_uri_title` Default: ` `

    Title to go with the `message_uri` value.

*   `message_priority` Default: ` `

    Priority level that will be used for the push notification.

*   `message_sound` Default: ` `

    Notification sound to play with the push notification.


### Conditions

*   `away_only` Default: `no`

    If set to `yes`, notifications will only be sent if the user has set
    their `/away` status, either directly from their client, or by using a
    ZNC module like `simple_away`.

*   `client_count_less_than` Default: `0`

    Notifications will only be sent if the number of connected IRC clients
    is less than this value.  A value of 0 (zero) will disable this condition.

*   `highlight` Default: ` `

    Space-separated list of highlight strings to match against channel
    messages using case-insensitive, wildcard matching.  Strings will bes
    compared in order they appear in the configuration value, and the first
    string to match will end the search, meaning that earlier strings take
    priority over later values.

    Individual strings may be prefixed with:

    *   `-` (hyphen) to negate the match, which makes the string act as a
        filter rather than a search

    *   `_` (underscore) to trigger a "whole-word" match, where it must be
        surrounded by whitespace to match the value

    *   `*` (asterisk) to match highlight strings that start with any of the
        above prefixes

    As an example, a highlight value of `-pinto car` will trigger notification
    on the message "I like cars", but will prevent notifications for
    "My favorite car is the Pinto" *and* "I like pinto beans".  Conversely,
    a highlight value of `car -pinto` will trigger notifications for the
    first two messages, and only prevent notification of the last one.

    As another example, a value of `_car` will trigger notification for the
    message "my car is awesome", but will not match the message "I like cars".

*   `idle` Default: `0`

    Time in seconds since the last activity by the user on any channel or
    query window, including joins, parts, messages, and actions.
    Notifications will only be sent if the elapsed time is greater than this
    value.  A value of 0 (zero) will disable this condition.

*   `last_active` Default: `180`

    Time in seconds since the last message sent by the user on that channel
    or query window.  Notifications will only be sent if the elapsed time is
    greater than this value.  A value of 0 (zero) will disable this condition.

    Note that this condition keeps track of the last message sent to each
    channel and query window separately, so a recent PM to Alice will not
    affect a notification sent from channel #foo.

*   `last_notification` Default: `300`

    Time in seconds since the last notification sent from that channel or
    query window.  Notifications will only be sent if the elapsed time is
    greater than this value.  A value of 0 (zero) will disable this condition.

    Note that this condition keeps track of the last notification sent from
    each channel and query window separately, so a recent PM from Alice will
    not affect a notification sent from channel #foo.

*   `nick_blacklist` Default: ` `

    Space-separated list of nicks.  Applies to both channel mentions and
    query windows.  Notifications will only be sent for messages from nicks
    that are not present in this list, using a case-insensitive comparison.

    Note that wildcard patterns can be used to match multiple nicks with a
    single blacklist entry.  For example, `set nick_blacklist *bot` will not
    send notifications from nicks like "channelbot", "FooBot", or "Robot".
    Care must be used to not accidentally blacklist legitimate nicks with
    wildcards.

*   `replied` Default: `yes`

    If set to `yes`, notifications will only be sent if you have replied to
    the channel or query window more recently than the last time a notification
    was sent for that context.


### Advanced

*   `channel_conditions` Default: `all`

    This option allows customization of the boolean logic used to determine
    how conditional values are used to filter notifications for channel
    messages.  It evaluates as a full boolean logic expression, including the
    use of sub-expressions.  The default value of `all` will bypass this
    evaluation and simply require all conditions to be true.

    The expression consists of space-separated tokens in the following grammar:

    *   expression = `<expression> <operator> <expression>` |
        `( <expression> )` | `<value>`
    *   operator = `and` | `or` | `is` | `==` | `!=`
    *   value = `true` | `false` | `none` | `<condition>` | `<context>` |
        `"<string>"` | `'<string>'`
    *   condition = `away_only` | `client_count_less_than` | `highlight` |
        `idle` | `last_active` | `last_notification` | `nick_blacklist` |
        `replied`
    *   context = `network` | `channel` | `nick` | `title` | `message`

    As a simple example, to replicate the default `all` value, would be the
    value of `away_only and client_count_less_than and highlight and idle and
    last_active and last_notification and nick_blacklist and replied`.

    Alternately, setting a value of `true` would send a notification for
    *every* message, while a value of `false` would *never* send a
    notification.

    For a more complicated example, the value of `client_count_less_than and
    highlight and (last_active or last_notification or replied) and
    nick_blacklist` would send a notification if any of the three conditions
    in the sub-expression are met, while still requiring all of the conditions
    outside of the parentheses to also be met.

*   `query_conditions` Default: `all`

    This option is more or less identical to `channel_conditions`, except that
    it is used to filter notifications for private messages.

*   `debug` Default: `off`

    When set to `on`, this option enables debug output for various features,
    and is useful in troubleshooting problems like failed push notifications.
    When set to `verbose`, even more debug output will be generated for every
    single incoming message, including details of why and when notifications
    get sent.  All debug output will show up in your `*push` query window.


License
-------

This project is copyright John Reese, and licensed under the MIT license.
I am providing code in this repository to you under an open source license.
Because this is my personal repository, the license you receive to my code is
from my and not from my employer.  See the `LICENSE` file for details.


[faq]: https://github.com/jreese/znc-push/blob/master/doc/faq.md
[examples]: https://github.com/jreese/znc-push/blob/master/doc/examples.md
[issues]: https://github.com/jreese/znc-push/issues

[Boxcar]: http://boxcar.io
[Notify My Android]: http://www.notifymyandroid.com
[Pushover]: http://pushover.net
[Prowl]: http://www.prowlapp.com
[Supertoasty]: http://www.supertoasty.com
[PushBullet]: https://www.pushbullet.com/
[Airgram]: http://airgramapp.com/
[Faast]: http://faast.io/

[ISO 8601]: http://en.wikipedia.org/wiki/ISO_8601 "ISO 8601 Date Format"

[ZNC]: http://en.znc.in "ZNC, an advanced IRC bouncer"
[modpython]: http://wiki.znc.in/Modpython
[requests]: http://docs.python-requests.org

<!-- vim:set ft= expandtab tabstop=4 shiftwidth=4: -->
