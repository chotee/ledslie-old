from collections import deque
from enum import Enum, unique

from twisted.python import log
from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import ServerFactory


class Protocol(LineReceiver):
    delimiter = b"\n"

    def lineReceived(self, data):
        data = data.decode("utf-8")
        op, data = data.split(None, 1)
        reply = self.factory.send_in(op, data, self)
        if reply:
            response = "%s %s" % ("reply", reply)
            return self.sendLine(response.encode('utf-8'))

    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)


class ProtocolFactory(ServerFactory):
    protocol = Protocol
    def __init__(self, service):
        self.service = service

    def send_in(self, op, contents, client):
        if op.lower() == 'register':
            return self.service.got_register(contents, client)
        else:
            method_name = "got_%s" % op.lower()
            method = getattr(self.service, method_name)
            return method(contents)

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")
