from enum import Enum

from twisted.internet import reactor
from twisted.internet.defer import Deferred
from twisted.internet.protocol import Factory
from twisted.internet.task import LoopingCall
from twisted.python import log
from twisted.protocols.basic import LineReceiver
from txzmq import ZmqEndpoint

from txzmq import ZmqFactory
from txzmq import ZmqRouterConnection


class Protocol(LineReceiver):
    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)


class ProtocolFactory(Factory):
    protocol = Protocol

    def __init__(self, service):
        super().__init__()
        self.service = service

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")


class ConnectionStateEnum(Enum):
    Disconnected = 0
    Connected = 1

class ConnectionWatchdog(object):
    def __init__(self, connection):
        self.connection = connection
        self.beat_frequency = 1.0  # every second
        self.setup_wait = 0.1 # seconds to wait before sending first heartbeat.
        self.allowed_beat_misses = 3  # How many beats to miss before flagging disconnect.
        self.count = 0

    def start_heartbeat(self, remote_identity):
        def _start(remote_identity):
            beat = LoopingCall(self.send_beat, remote_identity)
            beat.start(self.beat_frequency)
        reactor.callLater(self.setup_wait, _start, remote_identity)

    def send_beat(self, remote_identity):
        log.msg("Sending beat %d to remote identity %s" % (self.count, remote_identity))
        self.connection.sendMultipart(remote_identity, [b"ping", str(self.count).encode()])
        self.count += 1


class Connection(ZmqRouterConnection):
    def __init__(self, factory, endpoint=None, my_identity=None, remote_identity=None):
        """
        I am the connection to the other element.
        :param factory: The Connection Factory to use.
        :type factory: ZmqFactory
        :param endpoint: The endpoint to connect or bind to.
        :type endpoint: ZmqEndpoint
        :param my_identity: The Identity to use for the connection
        :type my_identity: str
        """
        my_identity = my_identity.encode() if my_identity else None
        remote_identity = remote_identity.encode() if remote_identity else None
        super().__init__(factory, endpoint, my_identity)
        self.state = ConnectionStateEnum.Disconnected
        self.connecting = (endpoint.type == 'connect')
        self.watchdog = ConnectionWatchdog(self)
        if self.connecting:
            assert remote_identity, "For connecting, a remote identity is required."
            self.watchdog.start_heartbeat(remote_identity)

    def established(self, sender_id):
        log.msg("connection to %s got established!" % sender_id)

    def disconnected(self, sender_id):
        log.msg("connection to %s got disconnected!" % sender_id)

    def gotMessage(self, sender_id, *frames):
        log.msg("gotMessage %s -> %s" % (sender_id, frames))


class ConnectionFactory(ZmqFactory):
    def __init__(self, reactor, protocol_factory):
        log.msg("Create %s" % self.__class__.__name__)
        self.reactor = reactor
        self.factory = protocol_factory
        self.registerForShutdown()
        super().__init__()

    def listen(self, uri, my_identity):
        """
        I listen to incoming connections.
        :param uri: The Port to bind to.
        :type uri: str
        :param my_identity: How others will address me.
        :type my_identity: str
        :return: None
        :rtype: None
        """
        self.connect(uri, method='bind', my_identity=my_identity)

    def connect(self, uri, method='connect', my_identity=None, remote_identity=None):
        """
        I make a network connection to others.
        :param uri: The URI I need to talk to
        :type uri: str
        :param method: Type of connection, can be "bind" (passive) or "connect" (active)
        :type method: str
        :param my_identity: How others will refer to me. Optional for "connect" methods
        :type my_identity: str
        :param remote_identity: The Id of others. Ignored for "bind" method connections.
        :type remote_identity: str
        :return: I get called when the connection becomes alive.
        :rtype: Deferred
        """
        endpoint = ZmqEndpoint(method, uri)
        conn = Connection(self, endpoint, my_identity, remote_identity)
        log.msg("connect: %s(%s) to %s(%s)" % (endpoint.type, my_identity,
                                               endpoint.address, remote_identity))
