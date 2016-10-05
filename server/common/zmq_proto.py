from twisted.internet.protocol import Factory
from twisted.python import log
from twisted.protocols.basic import LineReceiver
from txzmq import ZmqEndpoint

from txzmq import ZmqFactory
from txzmq import ZmqRouterConnection


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



    def startFactory(self):
        log.msg("startFactory called")


    def stopFactory(self):
        log.msg("stopFactory called")


class ConnectionFactory(ZmqFactory):
    def __init__(self, reactor):
        log.msg("Create %s" % self.__class__.__name__)
        self.reactor = reactor
        self.registerForShutdown()
        super().__init__()

    def listen(self, uri):
        self.connect(uri, method='bind')

    def connect(self, uri, method='connect'):
        e = ZmqEndpoint(method, uri)
        ZmqRouterConnection(self, e)
        log.msg("%s to %s" % (e.type, e.address))
