import sys
from twisted.internet.endpoints import TCP4ServerEndpoint

from swichboard.switchboard import SwitchboardFactory
from swichboard.manager import Starter

def main():
    from twisted.python import log
    log.startLogging(sys.stdout)
    from twisted.internet import reactor
    reactor.callWhenRunning(Starter())
    endpoint = TCP4ServerEndpoint(reactor, 8007)
    endpoint.listen(SwitchboardFactory())
    reactor.run()

if __name__ == "__main__":
    main()