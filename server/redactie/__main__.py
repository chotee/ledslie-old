import sys
from twisted.internet.endpoints import TCP4ServerEndpoint

from inbel import InbelFactory

def main():
    from twisted.python import log
    log.startLogging(sys.stdout)
#    redactie = Redactie()
    from twisted.internet import reactor
    endpoint = TCP4ServerEndpoint(reactor, 8007)
    endpoint.listen(InbelFactory())
    reactor.run()

if __name__ == "__main__":
    main()