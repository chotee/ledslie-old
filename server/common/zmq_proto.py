import time
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


class ConnectionWatchdog(object):
    """
    I help keeping watch over the senders connected to a zmq socket. I send them packets at regular intervals.
    """
    def __init__(self, connection):
        """
        Constructor
        :param connection: The connection of which senders need to be monitored.
        :type connection: Connection
        """
        self.connection = connection
        self.beat_frequency = 1.0  # every second
        self.setup_wait = 0.1 # seconds to wait before sending first heartbeat.
        self.allowed_beat_misses = 2  # How many beats to miss before flagging disconnect.
        self.grace_time = self.beat_frequency * self.allowed_beat_misses
        self.count = 0
        self.monitors = {}
        self.actives = set()

    def start_monitor(self, remote_identity):
        """
        I start the monitoring of a sender with remote_identity.
        :param remote_identity: The Identity of the sender.
        :type remote_identity: str
        """
        def _start_beats(remote_identity):
            beat = LoopingCall(self._send_beat, remote_identity)
            beat.start(self.beat_frequency)
        reactor.callLater(self.setup_wait, _start_beats, remote_identity)
        review = LoopingCall(self._review, remote_identity)
        review.start(self.grace_time)

    def _send_beat(self, remote_identity):
        """
        I send a single beat message to the sender with remote_identity via the self.connection object.
        :param remote_identity: The remote identity to send a beat to.
        :type remote_identity: str
        """
        #log.msg("Sending beat %d to remote identity %s" % (self.count, remote_identity))
        self.connection.sendMultipart(remote_identity, [b"ping", str(self.count).encode()])
        self.count += 1

    def _review(self, remote_identity):
        """
        I periodically review the state of a sender via a connection. If the connection is stale I will call
        sender_disconnected on self.connection.
        :param remote_identity: The identity to review.
        :type remote_identity: str
        """
        if remote_identity in self.monitors and self.monitors[remote_identity]+self.grace_time > time.time():
            return # Activity is good.
        if remote_identity in self.actives:
            self.actives.remove(remote_identity)
            self.connection.sender_disconnected(remote_identity)

    def report_activity(self, remote_identity):
        """
        I get called whenever there's activity of a sender.
        :param remote_identity: ID of the sender that had activity
        :type remote_identity: str
        """
        #log.msg("%s is alive" % remote_identity)
        self.monitors[remote_identity] = time.time()
        if remote_identity not in self.actives:
            self.actives.add(remote_identity)
            self.connection.sender_established(remote_identity)


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
        self.connecting = (endpoint.type == 'connect')
        self.watchdog = ConnectionWatchdog(self)
        self.senders = set()
        if self.connecting:
            assert remote_identity, "For connecting, a remote identity is required."
            self.register_sender(remote_identity)

    def register_sender(self, sender_id):
        """
        I register a sender with the system so that the connection will be monitored.
        :param sender_id: Identity of the sender to monitor
        :type sender_id: str
        """
        self.senders.add(sender_id)
        self.watchdog.start_monitor(sender_id)

    def sender_established(self, sender_id):
        """
        I get called when we can confirm a connection with a sender has been established.
        :param sender_id: The ID of the sender that we have established
        :type sender_id: str
        """
        log.msg("established connection to %s." % sender_id)

    def sender_disconnected(self, sender_id):
        """
        I get called when we have not received any traffic from a sender.
        :param sender_id: The ID of the sender that we have lost
        :type sender_id: str
        """
        log.msg("lost connection to %s" % sender_id)

    def gotMessage(self, sender_id, *frames):
        log.msg("gotMessage %s -> %s" % (sender_id, frames))
        if sender_id not in self.senders:
            log.msg("New sender %s" % sender_id)
            self.register_sender(sender_id)
        self.watchdog.report_activity(sender_id)


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
