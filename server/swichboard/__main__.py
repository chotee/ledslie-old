import sys
sys.path.insert(0, '.')

from common.zmq_proto import ProtocolFactory, ConnectionFactory
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
    connector = ConnectionFactory(reactor, factory)
    reactor.callWhenRunning(startServices([switchboard]))
    reactor.callWhenRunning(connector.listen, "tcp://*:8007", "switch")
    reactor.run()

if __name__ == "__main__":
    main()