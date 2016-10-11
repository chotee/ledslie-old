import sys
from twisted.python import log
from common.zmq_proto import ProtocolFactory, Rolodex
from common.connections import ConnectionFactory
from twisted.internet import reactor

def Main(service_cls):
    log.startLogging(sys.stdout)
    rolodex = Rolodex()
    service = service_cls(rolodex)
    factory = ProtocolFactory(rolodex, service)
    return ConnectionFactory(reactor, factory)
