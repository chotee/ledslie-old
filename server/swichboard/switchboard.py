from twisted.python import log
from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import Factory


class Switchboard(LineReceiver):
    def dataReceived(self, data):
#        role, msg = data.split()
        msg = data
        self.factory.new_message(msg)
        self.transport.write(msg[::-1])

    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)


class SwitchboardFactory(Factory):
    protocol = Switchboard
    def __init__(self):
        self._msg_queue = []

    def new_message(self, msg):
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
