import sys
from twisted.internet.endpoints import TCP4ServerEndpoint

from swichboard.proto import ProtocolFactory
from swichboard.switchboard import SwitchboardService


def main():
    from twisted.python import log
    log.startLogging(sys.stdout)
    switchboard = SwitchboardService()
    factory = ProtocolFactory(switchboard)
    from twisted.internet import reactor
    endpoint = TCP4ServerEndpoint(reactor, 8007)
    endpoint.listen(factory)
    reactor.run()

if __name__ == "__main__":
    main()