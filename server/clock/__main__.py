import sys

from twisted.internet import reactor
from twisted.internet.task import LoopingCall
from twisted.python import log
log.startLogging(sys.stdout)

sys.path.insert(0, '.')

from common.main import Main


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
    role = "clock"

    def __init__(self, rolodex):
        self.rolodex = rolodex
        time_ticker = LoopingCall(self.time_informer)
        reactor.callWhenRunning(time_ticker.start, 1)

    def time_informer(self):
        for remote in self.rolodex.all():
            #remote.sendMessage("send_second", "tick-tock")
            remote.sendRequest("reverse", "foo")

if __name__ == '__main__':
    connector = Main(ClockService)
    reactor.callWhenRunning(connector.connect, "tcp://127.0.0.1:8007", remote_identity="switch")
    reactor.run()