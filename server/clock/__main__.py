import sys
from sys import stdout
from datetime import datetime

from twisted.python import log
from twisted.internet.protocol import Protocol, ClientFactory

class Clock(Protocol):
    def dataReceived(self, data):
        stdout.write(data)
        from twisted.internet import reactor
        reactor.callLater(1, self._send_time)

    def connectionMade(self):
        self.transport.write("Hello!")
#        reactor.callLater(1000, self._send_time)

    def _send_time(self):
        self.transport.write(datetime.now().isoformat("T") + "\n")

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
    from twisted.internet import reactor
    log.msg("Starting up!")
    reactor.run()

if __name__ == '__main__':
    log.startLogging(sys.stdout)
    main()