import sys
from datetime import datetime

from twisted.internet import reactor
from twisted.python import log
from twisted.internet.protocol import Protocol, ClientFactory
from twisted.internet.endpoints import TCP4ClientEndpoint


class Clock(Protocol):
    def dataReceived(self, data):
        reactor.callLater(1, self._send_time)

    def connectionMade(self):
        self.transport.write(b"role reporter")
        reactor.callLater(1000, self._send_time)

    def _send_time(self):
        msg = "time %s\n" % datetime.now().isoformat("T")
        self.transport.write(msg.encode("utf-8"))


class ClockFactory(ClientFactory):
    def startedConnecting(self, connector):
        log.msg("Started to connect")

    def buildProtocol(self, addr):
        log.msg("Connected to %s" % addr)
        return Clock()

    def clientConnectionLost(self, connector, reason):
        log.msg('Lost connection. Reason:', reason)

    def clientConnectionFailed(self, connector, reason):
        log.msg('Connection failed. Reason:', reason)


def main():
    log.msg("Starting up!")
    endpoint = TCP4ClientEndpoint(reactor, "127.0.0.1", 8007)
    endpoint.connect(ClockFactory())
    reactor.run()

if __name__ == '__main__':
    log.startLogging(sys.stdout)
    main()