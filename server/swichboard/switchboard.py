from collections import deque

from twisted.python import log
from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import Factory


class Switchboard(LineReceiver):
    def dataReceived(self, data):
        op, msg = data.split()
        log.msg("data received: op=%s msg=%s" % (op, msg))
        if op == b'role':
            self.factory.set_role(self, msg)
        else:
            self.factory.new_message(self, msg)
        self.transport.write(msg[::-1])

    def connectionMade(self):
        log.msg("New connection")
        self.factory.new_connection(self)

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)
        self.factory.drop_connection(self)


class SwitchboardFactory(Factory):
    protocol = Switchboard

    def __init__(self):
        self._msg_queue = deque()
        self._conns = {}
        self._roles = {}

    def set_role(self, conn, role):
        self._roles.setdefault(role, []).append(conn)
        self._conns[conn] = role
        log.msg("connection %s has role %s" % (conn, role))

    def new_connection(self, conn):
        self._conns[conn] = None
        log.msg("After addition, Now has %d connections" % len(self._conns))

    def drop_connection(self, conn):
        del self._conns[conn]
        for role in self._roles.values():
            role.remove(conn)
        log.msg("After removal, Now has %d connections" % len(self._conns))

    def new_message(self, conn, msg):
        self._msg_queue.append(msg)
        log.msg(msg)
        log.msg("Queue now as %d messages!" % len(self._msg_queue))

    def has_messages(self):
        return bool(self._msg_queue)

    def get_message(self):
        return self._msg_queue.pop()

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")
