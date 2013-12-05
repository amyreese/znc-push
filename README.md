ZNC Push
========

ZNC Push is a module for [ZNC][] that will send notifications to multiple push notification
services for any private message or channel highlight that matches a configurable set of
conditions.  ZNC Push current supports the following services:

* [Boxcar][]
* [Notify My Android][] (NMA)
* [Pushover][]
* [Prowl][]
* [Supertoasty][]
* [PushBullet][]
* [im.kayac][]
* Custom URL GET requests

This project is still a Work In Progress, but should be functional enough and stable enough
for everyday usage.  Users are more than welcome to submit feature requests or patches for
discussion or inclusion.  Bug reports and feature requests can be submitted to
[the repository issues list][issues], or sent via email.

[![Stories in Ready](http://badge.waffle.io/jreese/znc-push.png)](http://waffle.io/jreese/znc-push)

For full functionality, this module requires ZNC version 0.090 or newer, but should compile
and run with a reduced feature set on versions as old as 0.078, the current version used by
Ubuntu.  However, development and testing is done exclusively against the latest source
distribution, so feedback on older releases of ZNC is needed to continue supporting them.
If you want to use ZNC versions before 1.0 (0.206 or older), you will need to check out the
"legacy" branch in order to compile it correctly.

ZNC Push was created by [John Reese](http://johnmreese.com) and designed to fill a
personal need.  It may not fit your use cases, but any and all feedback would be greatly
appreciated.

Dependencies
------------

If you have installed ZNC from a Linux distribution's repository, you will most likely
need to install the development package before building this module. On Ubuntu, this can
be installed with:

    $ sudo aptitude install znc-dev

Optionally, if you want to use libcurl for http requests, you also need to install cURL
development header files.

On Ubuntu, development headers can be installed by installing `libcurl3-dev` or
`libcurl4-openssl-dev` package:

    $ sudo aptitude install libcurl4-openssl-dev

Compiling
---------

If you have `make` installed, you can compile the module with:

    $ make

Otherwise, run the full command:

    $ znc-buildmod push.cpp


### Advanced

If you would like to compile ZNC Push using libcurl for http requests, you must use:

    $ make curl=yes

If libcurl is not in the default system library paths, you will need to populate `$CXXFLAGS`
with the appropriate GCC flags so that it can find and link ZNC Push with libcurl.

Note: You are strongly encouraged to use libcurl transport. The reason for that is, that
the default CSocket transport doesn't verify server's SSL certificate which leaves you
vulnerable to MITM attacks.

Installation
------------

Copy the compiled module into your ZNC profile:

    $ make install

Now, load the module in ZNC:

    /msg *status loadmod push

Then select the push service you want to use, and set your username and secret as needed.
The secret is not your password, and can be obtained by logging into the service's website
and looking in your profile or settings:

    /msg *push set service pushover
    /msg *push set username foo
	/msg *push set secret ...

If you're using Boxcar, you need to use the following command to send a subscription request
to your account, before ZNC Push can start working:

    /msg *push subscribe

At this point, it should start sending notifications every time you get a private message
or someone says your name in a channel.  If this is everything you wanted, congratulations,
you're done!

For further, detailed instructions specific to each push notification service, the following
documentation is available:

*   [Pushover](doc/pushover.md)


Commands
--------

*   `help`

    Links you to this fine document.

*   `version`

    Tells you the tagged build version currently running.

*   `set <option> <value>`

    Allows you to modify configuration values.

*   `append <option> <value>`

    Allows you to add a string to end of a configuration value.  Automatically adds a
    space to separate the appended value from the existing value.

*   `prepend <option> <value>`

    Allows you to add a string to beginning of a configuration value.  Automatically adds
    a space to separate the prepended value from the existing value.

*   `get [<option>]`

    Allows you to see current configuration values.

*   `unset <option>`

    Allows you to reset a configuration option back to the default value.

*   `save <filename>`

    Writes your options to a file with the given path and name.

*   `load <filename>`

    Loads your options from a file with the given path and name.  Caution should be taken,
    as this will lose any options that aren't already saved to the given file.

*   `status [<context>]`

    Check the status of current conditions.  Specifying the "context" of either a channel
    or nick name will provide status values specific to that context.

*   `subscribe`

    Send a subscription request for the selected service to your configured account.  This
    is required by certain services, such as Boxcar, before ZNC Push can send any messages
    to your account.

*   `send <message>`

    Manually trigger a notification with the given message.  Useful for testing to validate
    credentials, etc.

*   `eval <expression>`

    Evaluate the given expression in an empty context.  Useful for testing to validate that
    a given expression is properly formatted and does not contain invalid tokens.


Configuration
-------------

### Keyword Expansion

Some configuration options allow for optional keyword expansion, which happens
while preparing to send the push notification.  Expansion is performed each time
a notification is sent.  Expansion is only performed on options that explicitly


The following keywords will be replaced with the appropriate value:

*   `{context}`: the channel or query window context
*   `{nick}`: the nick that sent the message
*   `{datetime}`: [ISO 8601][] date string, in server-local time
*   `{unixtime}`: unix-style integer timestamp
*   `{title}`: the default title for the notification
*   `{message}`: the shortened message contents
*   `{username}`: the configured username string
*   `{secret}`: the configured secret string
*   `{password}`: the configured password string

As an example, a value of "http://domain/{context}/{datetime}" would be expanded
to something similar to "http://domain/#channel/2011-03-09 14:25:09", or
"http://domain/{nick}/{unixtime}" to "http://domain/somenick/1299685136".


### Push Services

*   `service = ""`

    Short name for the push notification service that you want to use.  Must be set before
    ZNC Push can send any notifications.

    Possible values include:

    *   "boxcar"
    *   "nma"
    *   "pushover"
    *   "prowl"
    *   "supertoasty"
    *   "pushbullet"
    *   "im.kayac"
    *   "url"

*   `username = ""`

    User account that should receive push notifications.

    This option must be set when using Boxcar or Pushover.

*   `secret = ""`

    Authentication token for push notifications.

    This option must be set when using Notify My Android, Pushover, Prowl, Supertoasty or PushBullet.

*   `target = ""`

    Device or target name for push notifications.

    When using Pushover, this option allows you to specify a single device name to send
    notifications to; if blank or unset, notifications will be sent to all devices.

    This option must be set when using PushBullet.

*   `password = ""`

    Authentication password for push notifications.

    This option can be set when using Notify im.kayac. (optional)

### Notifications

*   `message_content = "{message}"`

    Message content that will be sent for the push notification.  Keyword expansion is
    performed on this value.

*   `message_length = 100`

    Maximum length of the notification message to be sent.  The message will be nicely
    truncated and ellipsized at or before this length is reached.  A value of 0 (zero) will
    disable this option.

    When using the custom URL service, this options allows you to specify the URL to send
    a GET request to, and has keyword expansion performed on portions of it, including the
    path and any query parameter values.

*   `message_title = "{title}"`

    Title that will be provided for the push notification.  Keyword expansion is performed
    on this value.

*   `message_uri = ""`

    URI that will be sent with the push notification.  This could be a web address or a
    local scheme to access a mobile application.  Keyword expansion is performed on this
    value.

*   `message_uri_title` = ""`

    If you're using Pushover.net, you can specify a title for the `message_uri` option.

*   `message_priority = ""`

    Priority level that will be used for the push notification.
    Currently supported only by Pushover.net.

*   `message_sound = ""`

    Notification sound to play with the push notification.
    Currently only supported by Pushover.net.  Must be chosen from their list of
    [supported sounds](https://pushover.net/api#sounds).


### Conditions

*   `away_only = "no"`

    If set to "yes", notifications will only be sent if the user has set their `/away` status.

    This condition requires version 0.090 of ZNC to operate, and will be disabled when
    compiled against older versions.

*   `client_count_less_than = 0`

    Notifications will only be sent if the number of connected IRC clients is less than this
    value.  A value of 0 (zero) will disable this condition.

*   `highlight = ""`

    Space-separated list of highlight strings to match against channel messages using
    case-insensitive, wildcard matching.  Strings will be compared in order they appear in
    the configuration value, and the first string to match will end the search, meaning
    that earlier strings take priority over later values.

    Individual strings may be prefixed with:

    *   `-` (hypen) to negate the match, which makes the string act as a filter rather than
        a search

    *   `_` (underscore) to trigger a "whole-word" match, where it must be surrounded by
        whitespace to match the value

    *   `*` (asterisk) to match highlight strings that start with any of the above prefixes

    As an example, a highlight value of "-pinto car" will trigger notification on the
    message "I like cars", but will prevent notifications for "My favorite car is the Pinto"
    *and* "I like pinto beans".  Conversely, a highlight value of "car -pinto" will trigger
    notifications for the first two messages, and only prevent notification of the last one.

    As another example, a value of "_car" will trigger notification for the message "my car
    is awesome", but will not match the message "I like cars".

*   `idle = 0`

    Time in seconds since the last activity by the user on any channel or query window,
    including joins, parts, messages, and actions.  Notifications will only be sent if the
    elapsed time is greater than this value.  A value of 0 (zero) will disable this condition.

*   `last_active = 180`

    Time in seconds since the last message sent by the user on that channel or query window.
    Notifications will only be sent if the elapsed time is greater than this value.  A value
    of 0 (zero) will disable this condition.

    Note that this condition keeps track of the last message sent to each channel and query
    window separately, so a recent PM to Joe will not affect a notification sent from
    channel #foo.

*   `last_notification = 300`

    Time in seconds since the last notification sent from that channel or query window.
    Notifications will only be sent if the elapsed time is greater than this value.  A value
    of 0 (zero) will disable this condition.

    Note that this condition keeps track of the last notification sent from each channel and
    query window separately, so a recent PM from Joe will not affect a notification sent
    from channel #foo.

*   `nick_blacklist = ""`

    Space-separated list of nicks.  Applies to both channel mentions and query windows.
    Notifications will only be sent for messages from nicks that are not present in this
    list, using a case-insensitive comparison.

    Note that wildcard patterns can be used to match multiple nicks with a single blacklist
    entry. For example, `set nick_blacklist *bot` will not send notifications from nicks
    like "channelbot", "FooBot", or "Robot".  Care must be used to not accidentally
    blacklist legitimate nicks with wildcards.

*   `replied = "yes"`

    If set to "yes", notifications will only be sent if you have replied to the channel or
    query window more recently than the last time a notification was sent for that context.


### Advanced

*   `channel_conditions = "all"`

    This option allows customization of the boolean logic used to determine how conditional
    values are used to filter notifications for channel messages.  It evaluates as a full
    boolean logic expression,  including the use of sub-expressions.  The default value of
    "all" will bypass this evaluation and simply require all conditions to be true.

    The expression consists of space-separated tokens in the following grammar:

    *   expression = expression operator expression | "(" expression ")" | value
    *   operator = "and" | "or"
    *   value = "true" | "false" | condition
    *   condition = <any condition option>

    As a simple example, to replicate the default "all" value, would be the value of
    "away_only and client_count_less_than and highlight and idle and last_active and
    last_notification and nick_blacklist and replied".

    Alternately, setting a value of "true" would send a notification for *every* message,
    while a value of "false" would *never* send a notification.

    For a more complicated example, the value of "client_count_less_than and highlight and
    (last_active or last_notification or replied) and nick_blacklist" would send a
    notification if any of the three conditions in the sub-expression are met, while still
    requiring all of the conditions outside of the parentheses to also be met.

*   `query_conditions = "all"`

    This option is more or less identical to `channel_conditions`, except that it is used
    to filter notifications for private messages.

*   `debug = "off"`

    When set to "on", this option enables debug output for various features, and is useful
    in troubleshooting problems like failed push notifications.  Debug output will show up
    in your `*push` window.


License
-------

This project is licensed under the MIT license.  See the `LICENSE` file for details.



[Boxcar]: http://boxcar.io
[Notify My Android]: http://www.notifymyandroid.com
[Pushover]: http://pushover.net
[Prowl]: http://www.prowlapp.com
[Supertoasty]: http://www.supertoasty.com
[PushBullet]: https://www.pushbullet.com/
[im.kayac]: http://im.kayac.com/

[issues]: http://github.com/jreese/znc-push/issues
[ZNC]: http://en.znc.in "ZNC, an advanced IRC bouncer"
[ISO 8601]: http://en.wikipedia.org/wiki/ISO_8601 "ISO 8601 Date Format"

<!-- vim:set ft= expandtab tabstop=4 shiftwidth=4: -->
