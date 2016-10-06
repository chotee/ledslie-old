from twisted.internet.protocol import Factory
from twisted.protocols.basic import LineReceiver
from twisted.python import log


class Protocol(LineReceiver):
    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)


class ProtocolFactory(Factory):
    protocol = Protocol

    def __init__(self, service):
        super().__init__()
        self.service = service
        self.peers = {}

    def buildProtocol(self, addr):
        if addr not in self.peers:
            proto = super(ProtocolFactory, self).buildProtocol(addr)
            self.peers[addr] = proto
        else:
            proto = self.peers[addr]
        proto.connectionMade()

    def connection_lost(self, addr):
        self.peers[addr].connectionLost("disconnected")

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")


