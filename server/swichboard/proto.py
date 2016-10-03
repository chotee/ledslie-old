from collections import deque
from enum import Enum, unique

from twisted.python import log
from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import ServerFactory


class Protocol(LineReceiver):
    def lineReceived(self, data):
        data = data.decode("utf-8")
        op, data = data.split(None, 1)
        reply = self.factory.send_in(op, data)
        if reply:
            response = "%s %s" % ("reply", reply)
            self.sendLine(response.encode('utf-8'))
        return reply

    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)


class ProtocolFactory(ServerFactory):
    protocol = Protocol
    def __init__(self, service):
        self.service = service

    def send_in(self, op, contents):
        method_name = "got_%s" % op.lower()
        method = getattr(self.service, method_name)
        return method(contents)

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")
