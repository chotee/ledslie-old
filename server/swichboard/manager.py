from twisted.python import log
from twisted.internet import protocol
from twisted.internet import reactor

starters = [
    "python clock"
]

class Starter(object):
    def __call__(self):
        log.msg("I got called")
        servant_protocol = ServantProtocol()
        for starter in starters:
            reactor.spawnProcess(servant_protocol, starter, [starter])



class ServantProtocol(protocol.ProcessProtocol):
    def __init__(self):
        log.msg("%s got __init__" %self)

    def connectionMade(self):
        log.msg("%s got connectionMade" %self)
