from twisted.protocols.basic import NetstringReceiver

# class RedactieFactory()
#
# class Redactie(NetstringReceiver):
#     pass

from twisted.python import log
from twisted.internet.protocol import Protocol
from twisted.internet.protocol import Factory

class InbelFactory(Factory):
    def __init__(self):
        self._msg_queue = []

    def buildProtocol(self, addr):
        return Inbel(self)

    def new_message(self, msg):
        self._msg_queue.append(msg)
        log.msg("Queue now as %d messages!" % len(self._msg_queue))

    def has_messages(self):
        return bool(self._msg_queue)

    def get_message(self):
        return self._msg_queue.pop()


class Inbel(Protocol):
    def __init__(self, factory):
        self.factory = factory

    def dataReceived(self, data):
        if self.factory.has_messages():
            self.transport.write(self.factory.get_message())
        else:
            self.transport.write(data[::-1])
        self.factory.new_message(data)

    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connectioon list: %s", reason)
