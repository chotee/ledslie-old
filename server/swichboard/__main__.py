import sys
sys.path.insert(0, '.')

from twisted.internet import reactor
from twisted.python import log
log.startLogging(sys.stdout)

from common.main import Main
from swichboard.switchboard import SwitchboardService

if __name__ == "__main__":
    connector = Main(SwitchboardService)
    connector.listen("tcp://*:8007", "switch")
    reactor.run()
