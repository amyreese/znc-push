FAQ
===


### Why don't I get notifications?

Most users that are new to ZNC Push expect to receive a push notification any
time they get mentioned in a channel or receive a private message.  However,
by default, ZNC Push is configured to only send notifications when it's likely
that the user is either away from their computer, or not paying attention to
the conversation in question.  This will ideally prevent you from getting
"spammed" with notifications every time you get mentioned or receive a PM,
especially when you are actively participating in discussion.

If you don't like this default behavior, then take a look either at the
[conditions section][conditions] of the readme, or read through the
[example configuration guide][examples].  ZNC Push is highly configurable.

In some cases though, you may not receive notifications because your preferred
notification service is not configured correctly, or the service itself might
be having issues on their end.  The easiest way to verify that ZNC Push is
configured correctly is by sending a test message:

    /msg *push send Test message

If you see an error message like "secret not set", or something similar, then
you should verify that you've entered the appropriate value for that option.
The [push services section][services] of the readme has details on which
options are required for which notification services, and what values are
expected.



[conditions]: ../README.md#conditions
[services]: ../README.md#push-services
[examples]: examples.md
