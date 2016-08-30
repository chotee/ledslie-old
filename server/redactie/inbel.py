from twisted.python import log
from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import Factory


class Inbel(LineReceiver):
    def dataReceived(self, data):
        if self.factory.has_messages():
            self.transport.write(self.factory.get_message())
        else:
            self.transport.write(data[::-1])
        self.factory.new_message(data)

    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)


class InbelFactory(Factory):
    protocol = Inbel
    def __init__(self):
        self._msg_queue = []

    def new_message(self, msg):
        self._msg_queue.append(msg)
        log.msg("Queue now as %d messages!" % len(self._msg_queue))

    def has_messages(self):
        return bool(self._msg_queue)

    def get_message(self):
        return self._msg_queue.pop()

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")
