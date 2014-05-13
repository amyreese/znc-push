#
# ZNC Push Module
#
# Allows the user to enter a Push user and API token, and sends
# channel highlights and personal messages to Push.
#
# Copyright (c) 2014 John Reese
# Licensed under the MIT license
#

import json
import platform
import re
import time
import znc

from collections import defaultdict

try:
    import requests
    from requests import Request, Session
except ImportError:
    requests = None

VERSION = 'v2.0.0-rc'
USER_AGENT = 'ZNC Push/' + VERSION

C = None
T = None


class Context(object):
    stack = []

    def __init__(self, module, title=None, message=None,
                 nick=None, channel=None, network=None):
        self.module = module
        self.title = title
        self.message = message
        self.nick = nick
        self.channel = channel
        self.network = network
        self.timestamp = int(time.time())
        self.timestring = time.strftime('%Y-%m-%d %H:%M:%S',
                                        time.localtime(self.timestamp))

    def __enter__(self):
        Context.stack.append(self)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        assert Context.stack[-1] == self
        Context.stack.pop()

    @property
    def values(self):
        return {
            'title': self.title,
            'message': self.message,
            'nick': self.nick,
            'context': self.channel,
            'channel': self.channel,
            'network': self.network,
            'unixtime': self.timestamp,
            'datetime': self.timestring,
        }

    @classmethod
    def current(cls):
        if cls.stack:
            return cls.stack[-1]

        return None


class PushConfig(object):
    def __init__(self, module):
        self.module = module

        self.defaults = {
            # global options
            'lang': 'en_ca',

            # push service
            'service': '',
            'username': '',
            'secret': '',
            'target': '',

            # notification details
            'message_content': '{message}',
            'message_length': 100,
            'message_title': '{title}',
            'message_uri': '',
            'message_uri_title': '',
            'message_priority': '0',
            'message_sound': '',

            # conditions
            'away_only': 'no',
            'client_count_less_than': 0,
            'highlight': '',
            'idle': 0,
            'last_active': 180,
            'last_notification': 300,
            'nick_blacklist': '',
            'replied': 'yes',

            # advanced
            'channel_conditions': 'all',
            'query_conditions': 'all',
            'debug': 'off',
        }

        self.globals = {
            'lang',
            'debug',
        }

        self.absolutes = {
            'lang',
            'service',
            'away_only',
            'client_count_less_than',
            'idle',
            'last_active',
            'last_notification',
            'replied',
            'debug',
        }

        # todo: deserialize values from znc registry
        self.user_overrides = {}
        self.network_overrides = defaultdict(dict)
        self.channel_overrides = defaultdict(dict)

        self.load_config()

    def load_config(self):
        config_data = self.module.nv.get('config', None)
        if not config_data:
            return

        data = json.loads(config_data)

        for key, value in data['user'].items():
            self.user_overrides[key] = value

        for network, overrides in data['network'].items():
            for key, value in overrides.items():
                self.network_overrides[network][key] = value

        for channel, overrides in data['channel'].items():
            for key, value in overrides.items():
                self.channel_overrides[channel][key] = value

    def save_config(self):
        data = json.dumps({
                          'user': self.user_overrides,
                          'network': self.network_overrides,
                          'channel': self.channel_overrides,
                          })

        self.module.nv['config'] = data

    def dump(self):
        p = self.module.PutModule

        networks = self.network_overrides.keys()
        channels = self.channel_overrides.keys()

        kv = '{0}: {1}'
        no = '  /{0}: {1}'
        co = '  {0}: {1}'

        for key in sorted(self.defaults.keys()):

            if key in self.user_overrides:
                p(kv.format(key, self.user_overrides[key]))

            else:
                p(kv.format(key, self.defaults[key]))

            for network in networks:
                if key in self.network_overrides[network]:
                    p(no.format(network, self.network_overrides[network][key]))

            for channel in channels:
                if key in self.channel_overrides[channel]:
                    p(co.format(channel, self.channel_overrides[channel][key]))

    def get(self, key, network=None, channel=None):
        context = Context.current()

        if context:
            network = context.network
            channel = context.channel

        if key in self.globals:
            network = channel = None

        if (channel in self.channel_overrides
                and key in self.channel_overrides[channel]):
            return self.channel_overrides[channel][key]

        if (network in self.network_overrides
                and key in self.network_overrides[network]):
            return self.network_overrides[network][key]

        if key in self.user_overrides:
            return self.user_overrides[key]

        return self.defaults[key]

    def get_expanded(self, key):
        value = self.get(key)
        context = Context.current()

        if context:
            value = value.format(**context.values)

        return value

    def is_global(self, key):
        return key in self.globals

    def has_overrides(self, key):
        if key in self.globals:
            return False

        networks = self.network_overrides.keys()
        channels = self.channel_overrides.keys()

        for network in networks:
            if key in self.network_overrides[network]:
                return True

        for channel in channels:
            if key in self.channel_overrides[channel]:
                return True

        return False

    def set(self, key, value, network=None, channel=None):
        if key not in self.defaults:
            raise KeyError(T.e_option_not_valid.format(key))

        if key in self.globals:
            network = channel = None

        if key == 'lang' and value not in Translation.all_languages():
            raise ValueError(T.e_invalid_lang.format(value))

        if key == 'service' and value not in PushService.all_services():
            raise ValueError(T.e_unknown_service.format(value))

        t = type(self.defaults[key])

        if t == int:
            try:
                value = int(value)
            except ValueError:
                raise ValueError(T.e_option_not_int.format(key))

        if channel:
            self.channel_overrides[channel][key] = value

        elif network:
            self.network_overrides[network][key] = value

        else:
            self.user_overrides[key] = value

        self.save_config()

        return True

    def unset(self, key, network=None, channel=None):
        if key not in self.defaults:
            raise KeyError(T.e_option_not_valid.format(key))

        if key in self.globals:
            network = channel = None

        if channel and channel in self.channel_overrides:
            self.channel_overrides[channel].pop(key, None)

        elif network and network in self.network_overrides:
            self.network_overrides[network].pop(key, None)

        else:
            self.user_overrides.pop(key, None)

        self.save_config()

        return True

    def reset(self, key):
        if key not in self.defaults:
            raise KeyError(T.e_option_not_valid.format(key))

        self.user_overrides.pop(key, None)

        for channel, overrides in self.channel_overrides.items():
            overrides.pop(key, None)

        for network, overrides in self.network_overrides.items():
            overrides.pop(key, None)

        self.save_config()

        return True


class PushConditions(object):
    """Classifies message contexts and determines if push notifications should
    be triggered for each individual message."""

    def __init__(self, module):
        self.module = module

        self.idle_time = time.time()
        self.last_active_time = defaultdict(int)
        self.last_reply_time = defaultdict(int)
        self.last_notification_time = defaultdict(int)

    def PutDebug(self, message):
        self.module.PutDebug(message, verbose=True)

    def status(self, target=None):
        conditions = {
            'away_only': self.away_only(),
            'client_count_less_than': self.client_count_less_than(),
            'idle': self.idle(),
        }

        if target:
            conditions['last_active'] = self.last_active(target)
            conditions['last_notification'] = self.last_notification(target)
            conditions['replied'] = self.replied(target)

        return conditions

    def user_activity(self, target, action):
        """This method is called when the znc user is active in any channel
        or query context, including sending messages, joining, parting, etc."""

        now = time.time()
        self.idle_time = now
        self.last_active_time[target] = now

        if action in ('action', 'message'):
            self.last_reply_time[target] = now

    def push_channel(self, context):
        """This method is called for every incoming channel message, and must
        decide whether a push notification should be sent. A truthy return
        value will trigger a push notification."""

        self.PutDebug(T.d_eval_channel_message.format(
                      context.channel, context.nick, context.message))

        conditions = {
            'away_only': self.away_only(),
            'client_count_less_than': self.client_count_less_than(),
            'highlight': self.highlight(context.message),
            'idle': self.idle(),
            'last_active': self.last_active(context.channel),
            'last_notification': self.last_notification(context.channel),
            'nick_blacklist': self.nick_blacklist(context.nick),
            'replied': self.replied(context.channel),
        }

        for key, value in conditions.items():
            self.PutDebug(T.d_eval_term.format(key, value))

        expression = C.get('channel_conditions').lower()

        if expression != 'all':
            send = False
            conditions.update(true=True, false=False)

            try:
                self.PutDebug(T.d_eval_expression.format(expression))
                send = eval(expression, {'__builtins__': {},}, conditions)

            except SyntaxError as e:
                self.module.PutModule(T.e_eval_syntax.format(e))

            except Exception as e:
                self.module.PutModule(T.e_eval_exception.format(e))

        else:
            send = all(conditions.values())

        self.PutDebug(T.d_eval_result.format(T.yes if send else T.no))

        if send:
            self.last_notification_time[context.channel] = time.time()

        return send

    def push_query(self, context):
        """This method is called for every incoming query message, and must
        decide whether a push notification should be sent. A truthy return
        value will trigger a push notification."""

        self.PutDebug(T.d_eval_query_message.format(
                      context.nick, context.message))

        conditions = {
            'away_only': self.away_only(),
            'client_count_less_than': self.client_count_less_than(),
            'highlight': self.highlight(context.message),
            'idle': self.idle(),
            'last_active': self.last_active(context.nick),
            'last_notification': self.last_notification(context.nick),
            'nick_blacklist': self.nick_blacklist(context.nick),
            'replied': self.replied(context.nick),
        }

        for key, value in conditions.items():
            self.PutDebug(T.d_eval_term.format(key, value))

        expression = C.get('query_conditions').lower()

        if expression != 'all':
            send = False
            conditions.update(true=True, false=False)

            try:
                self.PutDebug(T.d_eval_expression.format(expression))
                send = eval(expression, {'__builtins__': {},}, conditions)

            except SyntaxError as e:
                self.module.PutModule(T.e_eval_syntax.format(e))

            except Exception as e:
                self.module.PutModule(T.e_eval_exception.format(e))

        else:
            send = all(conditions.values())

        self.PutDebug(T.d_eval_result.format(T.yes if send else T.no))

        if send:
            self.last_notification_time[context.channel] = time.time()

        return send

    def away_only(self):
        value = C.get('away_only')
        return value != 'yes' or self.module.GetNetwork().IsIRCAway()

    def client_count_less_than(self):
        # todo: fix GetClients() returning a SwigPyObject that we can't use
        return True

        value = C.get('client_count_less_than')
        network = self.module.GetNetwork()
        clients = network.GetClients()

        return value == 0 or len(clients) < value

    def idle(self):
        value = C.get('idle')
        now = time.time()
        return value == 0 or (now - self.idle_time) >= value

    def highlight(self, message):
        values = C.get('highlight').lower().split() + ['%nick%']
        message = ' {0} '.format(message.lower())

        for value in values:
            prefix = value[0]
            push = True

            if prefix == '-':
                value = value[1:]
                push = False

            elif prefix == '_':
                value = ' {0} '.format(value[1:])

            value = self.module.GetNetwork().ExpandString(value).lower()

            if value in message:
                return push

        return False

    def last_active(self, target):
        value = C.get('last_active')
        now = time.time()
        return value == 0 or (now - self.last_active_time[target]) >= value

    def last_notification(self, target):
        value = C.get('last_notification')
        now = time.time()
        return (value == 0
                or (now - self.last_notification_time[target]) >= value)

    def nick_blacklist(self, nick):
        values = C.get('nick_blacklist').lower().split()
        nick = nick.lower()

        for value in values:
            value = self.module.GetNetwork().ExpandString(value).lower()
            match = True

            if value.startswith('*'):
                match = match and nick.endswith(value[1:])

            if value.endswith('*'):
                match = match and nick.startswith(value[:-1])

            if not match:
                match = value == nick

            if match:
                return False

        return True

    def replied(self, target):
        value = C.get('replied')
        last_notification = self.last_notification_time[target]
        last_reply = self.last_reply_time[target]
        return (value != 'yes'
                or last_notification == 0
                or last_notification < last_reply)


class push(znc.Module):
    description = 'Send highlights and messages to a push notification service'
    module_types = [znc.CModInfo.UserModule]

    debug = True
    verbose = True
    conditions = None

    def UpdateGlobals(self):
        global T
        T = T.lang(C.get('lang'))

        debug = C.get('debug')
        self.debug = debug in ('on', 'verbose')
        self.verbose = debug == 'verbose'

    def PutDebug(self, message, verbose=True):
        if not self.debug or (verbose and not self.verbose):
            return

        if type(message) != str:
            message = str(message)

        self.PutModule(message)

    def PutVerbose(self, message):
        return self.PutDebug(message, verbose=True)

    def OnLoad(self, args, message):
        """Entry point of the module.  Initialize config, translations, and
        verify that required modules are installed."""

        global T, C
        T = Translation()
        C = PushConfig(self)

        self.UpdateGlobals()
        self.conditions = PushConditions(self)

        if requests is None:
            self.PutStatus(T.e_requests_missing)
            return False

        return True

    def OnChanMsg(self, nick, channel, message):
        network = self.GetNetwork().GetName()
        nick = nick.GetNick()
        channel = channel.GetName()
        message = str(message)

        with Context(self, title=T.channel_push, message=message,
                     nick=nick, channel=channel, network=network) as context:
            if self.conditions.push_channel(context):
                PushService.send_message(context)

        return znc.CONTINUE

    def OnChanAction(self, nick, channel, message):
        network = self.GetNetwork().GetName()
        nick = nick.GetNick()
        channel = channel.GetName()
        message = str(message)

        full_message = '* {0} {1}'.format(nick, message)
        with Context(self, title=T.channel_push, message=full_message,
                     nick=nick, channel=channel, network=network) as context:
            if self.conditions.push_channel(context):
                PushService.send_message(context)

        return znc.CONTINUE

    def OnPrivMsg(self, nick, message):
        network = self.GetNetwork().GetName()
        nick = nick.GetNick()
        message = str(message)

        with Context(self, title=T.query_push, message=message,
                     nick=nick, channel=None, network=network) as context:
            if self.conditions.push_query(context):
                PushService.send_message(context)

        return znc.CONTINUE

    def OnPrivAction(self, nick, message):
        network = self.GetNetwork().GetName()
        nick = nick.GetNick()
        message = str(message)

        full_message = '* {0} {1}'.format(nick, message)
        with Context(self, title=T.query_push, message=full_message,
                     nick=nick, channel=None, network=network) as context:
            if self.conditions.push_query(context):
                PushService.send_message(context)

        return znc.CONTINUE

    def OnUserMsg(self, target, message):
        target = str(target)
        self.conditions.user_activity(target, 'message')
        return znc.CONTINUE

    def OnUserAction(self, target, message):
        target = str(target)
        self.conditions.user_activity(target, 'action')
        return znc.CONTINUE

    def OnUserJoin(self, channel, key):
        channel = str(channel)
        self.conditions.user_activity(channel, 'join')
        return znc.CONTINUE

    def OnUserPart(self, channel, message):
        channel = str(channel)
        self.conditions.user_activity(channel, 'part')
        return znc.CONTINUE

    def OnUserTopic(self, channel, topic):
        channel = str(channel)
        self.conditions.user_activity(channel, 'topic')
        return znc.CONTINUE

    def OnUserTopicRequest(self, channel):
        channel = str(channel)
        self.conditions.user_activity(channel, 'topic')
        return znc.CONTINUE

    def OnModCommand(self, message):
        """Dispatches messages sent to *push to command functions."""

        tokens = message.split()

        if not tokens:
            return

        command = tokens[0].lower()
        method = getattr(self, 'cmd_' + command, None)

        if method is None:
            self.PutModule(T.e_invalid_command)
            return

        method(tokens[1:])

        if command in ('set', 'unset', 'reset'):
            self.UpdateGlobals()

        return znc.CONTINUE

    network_channel_value_re = re.compile(r'\s*(?:/([a-zA-Z0-9]+))?'
                                          r'\s*(#+[a-zA-Z0-9]+)?\s*(.*)')

    def parse_network_channel_value(self, tokens):
        """Given a list of tokens sent to a command function, uses a regex
        to parse out the optional network and channel predicates for commands
        that require working with configuration values."""

        message = ' '.join(tokens)
        match = self.network_channel_value_re.match(message)
        self.PutDebug(match.groups())

        if match:
            return match.group(1), match.group(2), match.group(3)

        else:
            return None, None, message

    def cmd_help(self, tokens):
        """Print some help text."""

        self.PutModule(T.help_website)

    def cmd_version(self, tokens):
        """Print the current version of znc-push, as well as the version of
        python that it's running on."""

        s = 'znc-push {0}, python {1}'
        self.PutModule(s.format(VERSION, platform.python_version()))

    def cmd_dump(self, tokens):
        """Print a list of all configuration options, listing all overrides
        for networks and channels as well as the global/user value."""

        C.dump()

    def cmd_get(self, tokens):
        """Print the configured value for one or more options, following any
        given network/channel predicates.  Keys with overrides will be printed
        with an asterisk.  Global options will be printed with an @ symbol."""

        network, channel, keys = self.parse_network_channel_value(tokens)

        if not keys or keys == 'all':
            keys = C.defaults.keys()

        else:
            keys = keys.split()

        m = '{0}{1} {2}'
        for key in sorted(keys):
            try:
                value = C.get(key, network=network, channel=channel)
                modifier = ''

                if C.is_global(key):
                    modifier += '@'

                if C.has_overrides(key):
                    modifier += '*'

                self.PutModule(m.format(key, modifier, value))
            except KeyError:
                self.PutModule(T.e_option_not_valid.format(key))

    def cmd_set(self, tokens):
        """Modify a configuration option to the given value, following any
        given network/channel predicates."""

        network, channel, tokens = self.parse_network_channel_value(tokens)
        tokens = tokens.split()
        key = tokens[0]
        value = ' '.join(tokens[1:])

        try:
            C.set(key, value, network=network, channel=channel)
            self.PutModule(T.done)

        except KeyError:
            self.PutModule(T.e_option_not_valid.format(key))

        except ValueError:
            self.PutModule(T.e_option_not_int)

    def cmd_append(self, tokens):
        """Modify a configuration option to append the given value, following
        any given network/channel predicates."""

        network, channel, tokens = self.parse_network_channel_value(tokens)
        tokens = tokens.split()
        key = tokens[0]
        value = ' '.join(tokens[1:])

        try:
            orig = C.get(key, network=network, channel=channel)
            new_value = ' '.join((orig, value))

            C.set(key, new_value, network=network, channel=channel)
            self.PutModule(T.done)

        except KeyError:
            self.PutModule(T.e_option_not_valid.format(key))

        except ValueError:
            self.PutModule(T.e_option_not_int)

    def cmd_prepend(self, tokens):
        """Modify a configuration option to prepend the given value, following
        any given network/channel predicates."""

        network, channel, tokens = self.parse_network_channel_value(tokens)
        tokens = tokens.split()
        key = tokens[0]
        value = ' '.join(tokens[1:])

        try:
            orig = C.get(key, network=network, channel=channel)
            new_value = ' '.join((value, orig))

            C.set(key, new_value, network=network, channel=channel)
            self.PutModule(T.done)

        except KeyError:
            self.PutModule(T.e_option_not_valid.format(key))

        except ValueError:
            self.PutModule(T.e_option_not_int)

    def cmd_unset(self, tokens):
        """Remove a configuration option, following any given network/channel
        predicates.  Only removes a single override, or global value.  Use
        the reset command to remove all override values as well."""

        network, channel, keys = self.parse_network_channel_value(tokens)
        keys = keys.split()

        for key in keys:
            try:
                C.unset(key, network=network, channel=channel)
            except KeyError:
                self.PutModule(T.e_option_not_valid.format(key))

        self.PutModule(T.done)

    def cmd_reset(self, tokens):
        """Remove a configuration option from all user, channel, and network
        overrides, returning to a completely default value.  Use the unset
        command to only remove an individual override."""

        for key in tokens:
            try:
                C.reset(key)
            except KeyError:
                self.PutModule(T.e_option_not_valid.format(key))

        self.PutModule(T.done)

    def cmd_status(self, tokens):
        network, channel, tokens = self.parse_network_channel_value(tokens)

        cv = '{0}: {1}'

        conditions = self.conditions.status(target=channel)
        for key in sorted(conditions.keys()):
            self.PutModule(cv.format(key, conditions[key]))

    def cmd_subscribe(self, tokens):
        """Subscribe the user to their currently configured push service,
        following any given network/channel predicates."""

        network, channel, message = self.parse_network_channel_value(tokens)

        with Context(self, network=network, channel=channel) as context:
            PushService.send_subscribe(context)

        self.PutModule(T.done)

    def cmd_send(self, tokens):
        """Send a test message to the user's currently configured push service,
        following any given network/channel predicates."""

        network, channel, message = self.parse_network_channel_value(tokens)

        network = network or '*push'
        channel = channel or '*push'

        with Context(self, title=T.test_message, message=message, nick='*push',
                     channel=channel, network=network) as context:
            PushService.send_message(context)

        self.PutModule(T.done)


class PushService(object):
    """Framework for adding new push service implementations, by subclassing
    PushService and implementing send(). The framework expects unprepared
    Request objects, that it can potentially modify for proxy info, or execute
    on a separate thread."""

    required = {}
    """A dictionary of config values that are required by the push service.
    Keys are the config options themselves, and the values are strings that
    describe what the option should contain.  Eg, 'secret': 'API token'."""

    def send(self, context):
        """Send a push notification for the given context.
        This must return an unprepared Request object, which the framework will
        handle and execute.  If the return type is None, then the framework
        will assume a network request is not needed; otherwise, if the return
        type is not either a Request object or None, then the framework will
        treat this as an error."""

        # error
        return False

    def subscribe(self, context):
        """Subscribe the user to their currently-configured push service.
        For services that require subscribing before notifications can be
        received, this should return an unprepared Request object.  If this is
        not required by the push service, then return None; otherwise, if the
        return type is not either a Request object or None, then the framework
        will treat this as an error."""

        # not required
        return None

    @classmethod
    def send_message(cls, context):
        service_name = C.get('service')
        service = cls.service(service_name)

        if service is None:
            context.module.PutModule(T.e_unknown_service.format(service_name))
            return

        request = service.send(context)

        if request is None:
            return

        elif type(request) != Request:
            context.module.PutModule(T.e_bad_push_handler)
            return

        session = Session()
        prepped = session.prepare_request(request)
        response = session.send(prepped, verify=True)

        if response.status_code != 200:
            m = T.e_send_status
            context.module.PutModule(m.format(response.status_code))

        for line in response.text.split('\n'):
            context.module.PutDebug(line)

    @classmethod
    def send_subscribe(cls, context):
        service_name = C.get('service')
        service = cls.service(service_name)

        if service is None:
            context.module.PutModule(T.e_unknown_service.format(service_name))
            return

        request = service.subscribe(context)

        if request is None:
            m = T.e_no_subscribe
            context.module.PutModule(m.format(service))
            return

        elif type(request) != Request:
            context.module.PutModule(T.e_bad_push_handler)
            return

        session = Session()
        prepped = session.prepare_request(request)
        response = session.send(prepped, verify=True)

        if response.status_code != 200:
            m = T.e_send_subscribe
            context.module.PutModule(m.format(response.status_code))

        for line in response.text.split('\n'):
            context.module.PutDebug(line)

    _cache = None

    @classmethod
    def all_services(cls):
        if cls._cache is None:
            cls._cache = {}
            queue = {cls}

            while queue:
                parent = queue.pop()
                cls._cache[parent.__name__.lower()] = parent()

                for child in parent.__subclasses__():
                    queue.add(child)

        return cls._cache.copy()

    @classmethod
    def service(cls, name):
        services = cls.all_services()

        if name in services:
            return services[name]

        return None


class Pushover(PushService):
    required = {
        'secret': 'API key/token',
        'username': 'User key',
    }

    def send(self, context):
        url = 'https://api.pushover.net/1/messages.json'

        message_uri = C.get_expanded('message_uri')
        message_uri_title = C.get_expanded('message_uri_title')
        message_sound = C.get('message_sound')
        message_priority = C.get('message_priority')
        target = C.get('target')

        params = {
            'token': C.get('secret'),
            'user': C.get('username'),
            'message': C.get_expanded('message_content'),
            'title': C.get_expanded('message_title'),
        }

        if message_uri:
            params['message_uri'] = message_uri

        if message_uri_title:
            params['message_uri_title'] = message_uri_title

        if message_sound:
            params['message_sound'] = message_sound

        if message_priority:
            params['message_priority'] = message_priority

        if target:
            params['target'] = target

        return Request('POST', url, data=params)


class Translation(object):
    _cache = None

    @classmethod
    def all_languages(cls):
        if cls._cache is None:
            cls._cache = {}
            queue = {cls}

            while queue:
                parent = queue.pop()
                if parent._lang:
                    cls._cache[parent._lang.lower()] = parent()

                for child in parent.__subclasses__():
                    queue.add(child)

        return cls._cache.copy()

    @classmethod
    def lang(cls, name):
        langs = cls.all_languages()
        name = name.lower()

        if name in langs:
            return langs[name]

        return langs[Translation._lang]

    _lang = 'en_us'

    yes = 'yes'
    no = 'no'
    done = 'done'
    help_website = 'View the detailed documentation at '\
                   'https://github.com/jreese/znc-push/blob/master/README.md'

    test_message = 'Test message'
    channel_push = 'Highlight'
    query_push = 'Private Message'

    d_eval_channel_message = 'Evaluating channel message {0} <{1}> {2}'
    d_eval_query_message = 'Evaluating query message <{0}> {1}'
    d_eval_term = '  {0}: {1}'
    d_eval_expression = 'Evaluating custom expression: {0}'
    d_eval_result = 'Send push notification? {0}'

    e_requests_missing = 'Error: could not import python requests module'
    e_invalid_command = 'Error: invalid command, try `help`'
    e_invalid_lang = 'Sorry, {0} is not supported. Translation help is welcome'
    e_option_not_valid = 'Error: invalid option name "{0}"'
    e_option_not_int = 'Error: option "{0}" requires integer value'

    e_unknown_service = 'Error: push service {0} not supported'
    e_send_status = 'Error: status {0} while sending message'
    e_bad_push_handler = 'Error: no request returned from handler'
    e_no_subscribe = 'No need to subscribe for {0}'
    e_send_subscribe = 'Error: status {0} while subscribing'

    e_eval_exception = 'Error evalutaing custom expression: {0}'
    e_eval_syntax = 'Syntax error in custom expression: {0}'


class Canadian(Translation):
    _lang = 'en_ca'

    done = 'okay'

    e_requests_missing = 'Sorry, could not import python requests module'
    e_invalid_command = 'Sorry, invalid command, try `help`'
    e_invalid_lang = 'Sorry, {0} is not supported. Translation help is welcome'
    e_option_not_valid = 'Sorry, invalid option name "{0}"'
    e_option_not_int = 'Sorry, option "{0}" requires integer value'

    e_unknown_service = 'Sorry, push service {0} not supported'
    e_send_status = 'Sorry, status {0} while sending message'
    e_bad_push_handler = 'Sorry, no request returned from handler'
    e_no_subscribe = 'Sorry, no need to subscribe for {0}'
    e_send_subscribe = 'Sorry, status {0} while subscribing'
