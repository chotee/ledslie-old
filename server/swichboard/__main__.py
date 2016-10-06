import sys
sys.path.insert(0, '.')

from common.zmq_proto import ProtocolFactory
from common.connections import ConnectionFactory
from swichboard.switchboard import SwitchboardService

from twisted.internet import reactor
from twisted.python import log

log.startLogging(sys.stdout)

def startServices(objects_to_start):
    def _call():
        for obj in objects_to_start:
            obj.start()
    return _call

def main():
    switchboard = SwitchboardService()
    factory = ProtocolFactory(switchboard)
    connector = ConnectionFactory(reactor, factory)
    reactor.callWhenRunning(startServices([switchboard]))
    d = connector.listen("tcp://*:8007", "switch")
    d.addCallback(lambda msg: log.msg("Successfully talking"))
    d.addErrback(lambda err: reactor.stop())
    log.msg("Reactor start")
    reactor.run()
    log.msg("Reactor stop")

if __name__ == "__main__":
    main()