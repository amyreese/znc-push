Example Configurations
======================


Default Behavior
----------------

Most users that are new to ZNC Push expect to receive a push notification any
time they get mentioned in a channel or receive a private message.  However,
by default, ZNC Push is configured to only send notifications when it's likely
that the user is either away from their computer, or not paying attention to
the conversation in question.  This will ideally prevent you from getting
"spammed" with notifications every time you get mentioned or receive a PM,
especially when you are actively participating in discussion.

### Configuration

    channel_conditions: all
    query_conditions: all

    away_only: no
    client_count_less_than: 0
    highlight:
    idle: 0
    last_active: 180
    last_notification: 300
    replied: yes

### Explanation

The use of `channel_conditions: all` and `query_conditions: all` means that
ZNC Push will evaluate all of the other conditions, and require that all of
them "pass" in order for a notification to be sent.

But the real key to understanding the default configuration is the use of
`last_active`, `last_notification`, and `replied` to try and determine if the
user is "active" in the conversation whenever they get highlighted or receive
a private message.

* `last_active` looks for whether the user has performed an action that would
  generall require direct involvement, such as sending a message to a channel,
  sending a private message, or sending an action (`/me ...`).  This is
  specific to the context in which the user receives a message. So by default,
  if the user gets highlighted or receives a private message, a notification
  will only get sent if the user hasn't sent a message to that context in the
  last 180 seconds (three minutes).

* `last_notification` looks for the last time that a notification was sent to
  the user.  This is specific to the context in which a user receives a
  message.  So by default, if the user gets highlighted or receives a private
  message, they will only get a notification if they haven't already received
  a notification for that context within the last 300 seconds (five minutes).

* `replied` looks to see if the user has responded since the last time a
  notification was sent.  This is specific to the context in which the user
  receives a message.  By default, if the user gets highlighted or receives a
  private message, a notification is only sent if the user has not yet received
  a notification for that context, or if they have gotten a notification and
  have since responded to that context.


Show Me Everything
------------------

Sometimes you just want to see everything.  More is better.  You want ZNC Push
to work more like Colloquy or Growl.  You want a push notification every single
time you get mentioned, and every time you get a private message.  And maybe
you don't care about nickserv.

That's easy!

### Configuration

    channel_conditions: highlight and nick_blacklist
    query_condition: nick_blacklist

    nick_blacklist: nickserv

### Explanation

By overriding `channel_conditions` and `query_conditions`, you're telling
ZNC Push that you only care about a specific set of checks when sending
notifications, and that you want to ignore the rest.

Both of these options are [boolean expressions][booleans], which evaluate to
either true or false.  If it evaluates to true, then a notification is sent.
By specifying `highlight and nick_blacklist`, you're telling ZNC Push that
only those two conditions must be true in order to send a notification.

In this case, `highlight` is true for any message that includes either your
nick, or a word that's listed in your configured value for the `highlight`
option.  Similarly, `nick_blacklist` is true for any message that *isn't* sent
by a user that's listed in your configured value for the `nick_blacklist`
option.  Since highlight is relatively meaningless for private messages,
it can safely be left out of `query_conditions`, leaving just `nick_blacklist`.

The result of this example is that you'll receive a push notification for each
and every private message -- that doesn't come from nickserv -- and every time
you get mentioned in a channel by someone who isn't nickserv.


[booleans]: https://en.wikipedia.org/wiki/Boolean_expression
