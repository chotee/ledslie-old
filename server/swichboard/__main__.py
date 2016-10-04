import sys
from twisted.internet.endpoints import TCP4ServerEndpoint

from swichboard.proto import ProtocolFactory
from swichboard.switchboard import SwitchboardService

def startServices(objects_to_start):
    def _call():
        for obj in objects_to_start:
            obj.start()
    return _call

def main():
    from twisted.python import log
    log.startLogging(sys.stdout)
    switchboard = SwitchboardService()
    factory = ProtocolFactory(switchboard)
    from twisted.internet import reactor
    endpoint = TCP4ServerEndpoint(reactor, 8007)
    endpoint.listen(factory)
    reactor.callWhenRunning(startServices([switchboard]))
    reactor.run()

if __name__ == "__main__":
    main()