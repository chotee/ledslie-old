import sys
from datetime import datetime

sys.path.insert(0, '.')

#from twisted.internet import reactor
from twisted.python import log
#from twisted.internet.protocol import Protocol, ClientFactory
from common.zmq_proto import ProtocolFactory, ConnectionFactory
from twisted.internet.endpoints import TCP4ClientEndpoint


# class ClockProtocol(Protocol):
#     def connectionMade(self):
#         self.transport.write(b"role reporter")
#         reactor.callLater(1, self._send_time)
#
#     def _send_time(self):
#         msg = "time %s\n" % datetime.now().isoformat("T")
#         self.transport.write(msg.encode("utf-8"))
#         reactor.callLater(1, self._send_time)
#
#
# class ClockProtocolFactory(ClientFactory):
#     def startedConnecting(self, connector):
#         log.msg("Started to connect")
#
#     def buildProtocol(self, addr):
#         log.msg("Connected to %s" % addr)
#         return ClockProtocol()
#
#     def clientConnectionLost(self, connector, reason):
#         log.msg('Lost connection. Reason:', reason)
#
#     def clientConnectionFailed(self, connector, reason):
#         log.msg('Connection failed. Reason:', reason)


class ClockService(object):
    pass

def main():
    log.startLogging(sys.stdout)
    clock_service = ClockService()
    factory = ProtocolFactory(clock_service)
    from twisted.internet import reactor
    connector = ConnectionFactory(reactor, factory)
    reactor.callWhenRunning(connector.connect, "tcp://127.0.0.1:8007", remote_identity="switch")
    reactor.run()

if __name__ == '__main__':
    log.startLogging(sys.stdout)
    main()