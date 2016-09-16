import sys

from twisted.internet import reactor
from twisted.python import log
from twisted.internet.protocol import Protocol, ClientFactory
from twisted.internet.endpoints import TCP4ClientEndpoint


class Layout(Protocol):
    def dataReceived(self, data):
        log.msg("Got data: %s" % data)

    def connectionMade(self):
        self.transport.write(b"role layout")

class LayoutFactory(ClientFactory):
    protocol = Layout

    def startedConnecting(self, connector):
        log.msg("Started to connect")

    def clientConnectionLost(self, connector, reason):
        log.msg('Lost connection. Reason:', reason)

    def clientConnectionFailed(self, connector, reason):
        log.msg('Connection failed. Reason:', reason)


def main():
    log.msg("Starting up!")
    endpoint = TCP4ClientEndpoint(reactor, "127.0.0.1", 8007)
    endpoint.connect(LayoutFactory())
    reactor.run()

if __name__ == '__main__':
    log.startLogging(sys.stdout)
    main()