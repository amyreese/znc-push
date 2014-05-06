#
# ZNC Push Module
#
# Allows the user to enter a Push user and API token, and sends
# channel highlights and personal messages to Push.
#
# Copyright (c) 2014 John Reese
# Licensed under the MIT license
#

import os
import platform
import znc

try:
    import requests
except ImportError:
    requests = None

VERSION = 'dev'
USER_AGENT = 'ZNC Push/' + VERSION


class Context(object):
    stack = []

    def __init__(self, module, message, channel, sender):
        self.module = module
        self.message = message
        self.channel = channel
        self.sender = sender

    def __enter__(self):
        Context.stack.append(self)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        assert Context.stack[-1] == self
        Context.stack.pop()

    @classmethod
    def current(cls):
        if stack:
            return stack[-1]

        return None


class push(znc.Module):
    description = 'Send highlights and messages to a push notification service'
    module_types = [znc.CModInfo.NetworkModule, znc.CModInfo.UserModule]

    debug = True

    def PutDebug(self, message):
        if self.debug:
            self.PutStatus(message)

    def OnLoad(self, args, message):
        if requests is None:
            self.PutStatus('Error: could not import python requests module')
            return False

        return True

    def OnModCommand(self, message):
        tokens = message.split()

        if not tokens:
            return

        command = 'cmd_' + tokens[0].lower()
        method = getattr(self, command, None)

        if method is None:
            self.PutModule('Error: invalid command, try `help`')
            return

        method(tokens)

    def cmd_help(self, tokens):
        self.PutModule('View the detailed documentation at '
                       'https://github.com/jreese/znc-push/blob/master/README.md')

    def cmd_version(self, tokens):
        s = 'znc-push {0}, python {1}'
        self.PutModule(s.format(VERSION, platform.python_version()))

    def cmd_send(self, tokens):
        message = ' '.join(tokens)

        with Context(self, message, '*push', '*push') as c:
            PushService.send_message()

        self.PutModule('done')


class PushService(object):
    required = {}

    def send(self, context):
        return

    @classmethod
    def send_message(cls):
        context = Context.current()
        if context is None:
            raise ValueError('No current context')

        request = cls.service('pushover').send(context)

        if not request:
            module.PutModule('Error: no request returned from handler')
            return

        if request.status_code != 200:
            m = 'Error: status {0} while sending message'
            module.PutModule(m.format(request.status_code))

        for line in request.text.split('\n'):
            module.PutDebug(request.text)

    _cache = None
    @classmethod
    def service(cls, name):
        if _cache is None:
            queue = {cls}

            while queue:
                parent = queue.pop()
                _cache[parent.name.lower()] = parent()

                for child in parent.subclasses():
                    queue.add(child)

            return _cache.copy()

        if name in _cache:
            return _cache[key]

        return None


class Pushover(PushService):
    required = {
        'secret': 'API key/token',
        'username': 'User key',
    }

    def send(self, context):
        url = 'https://api.pushover.net/1/messages.json'

        params = {
            'token': '',
            'user': '',
            'message': context.message,
            'title': 'test'
        }

        return requests.post(url, params=params)
