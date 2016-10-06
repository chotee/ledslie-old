import time

import zmq
from twisted.internet import reactor
from twisted.internet.defer import Deferred
from twisted.internet.task import LoopingCall
from twisted.python import log
from twisted.python.failure import Failure
from zmq import constants as zmq_constants
from txzmq import ZmqRouterConnection, ZmqFactory, ZmqEndpoint


class ConnectionWatchdog(object):
    """
    I help keeping watch over the senders connected to a zmq socket. I send them packets at regular intervals.
    """
    def __init__(self, connection, ):
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
        self.monitors = {}
        self.last_seen = {}
        self.actives = set()

    def start_monitor(self, remote_identity):
        """
        I start the monitoring of a sender with remote_identity.
        :param remote_identity: The Identity of the sender.
        :type remote_identity: str
        """
        self.last_seen[remote_identity] = time.time()
        if remote_identity not in self.monitors:
            self.monitors[remote_identity] = {
                'review': LoopingCall(self._review, remote_identity),
                'beat': LoopingCall(self._send_beat, remote_identity),
                'count': 0
            }
            self._start_checkloops(remote_identity)

    def _start_checkloops(self, remote_identity):
        beat = self.monitors[remote_identity]['beat']
        review = self.monitors[remote_identity]['review']
        def _start_beats(beat):
            if not beat.running:
                beat.start(self.beat_frequency)
        reactor.callLater(self.setup_wait, _start_beats,  beat)
        if not review.running:
            review.start(self.grace_time)

    def _send_beat(self, remote_identity):
        """
        I send a single beat message to the sender with remote_identity via the self.connection object.
        :param remote_identity: The remote identity to send a beat to.
        :type remote_identity: str
        """
        count = self.monitors[remote_identity]['count']
        #log.msg("Sending beat %d to remote identity %s" % (count, remote_identity))
        self.connection.sendMultipart(remote_identity, [b"ping", str(count).encode()])
        self.monitors[remote_identity]['count'] += 1

    def _review(self, remote_identity):
        """
        I periodically review the state of a sender via a connection. If the connection is stale I will call
        sender_disconnected on self.connection.
        :param remote_identity: The identity to review.
        :type remote_identity: str
        """
        last_seen = self.last_seen[remote_identity]
        if last_seen + self.grace_time > time.time():
            return  # Still active.
        if remote_identity in self.actives:
            self.actives.remove(remote_identity)
            self.monitors[remote_identity]['beat'].stop()
            self.monitors[remote_identity]['review'].stop()
            self.connection.sender_disconnected(remote_identity)

    def report_activity(self, remote_identity):
        """
        I get called whenever there's activity of a sender.
        :param remote_identity: ID of the sender that had activity
        :type remote_identity: str
        """
        #log.msg("%s is alive" % remote_identity)
        self.last_seen[remote_identity] = time.time()
        if remote_identity not in self.actives:
            self.actives.add(remote_identity)
            self._start_checkloops(remote_identity)
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
        log.msg("Building connection for %s [%s]" % (remote_identity, endpoint))
        super().__init__(factory, endpoint, my_identity)
        self.socket.set(zmq_constants.PROBE_ROUTER, 1)
        self.connecting = (endpoint.type == 'connect')
        self.watchdog = ConnectionWatchdog(self)
        self.senders = set()
        self.protocols = {}
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
        protocol = self.factory.connection_established(self, sender_id)
        self.protocols[sender_id] = protocol

    def sender_disconnected(self, sender_id):
        """
        I get called when we have not received any traffic from a sender.
        :param sender_id: The ID of the sender that we have lost
        :type sender_id: str
        """
        log.msg("lost connection to %s" % sender_id)
        self.factory.connection_lost(sender_id)

    def gotMessage(self, sender_id, *frames):
        """
        I get called when a message is received on the socket. If the message is a ping, I inform the watchdog.
        If it's any other, I pass it on to the object maintaining state with the recipient.
        :param sender_id: The Id of the sender having send the connection
        :type sender_id: str
        :param frames: All the frames of the message.
        :type frames: tuple
        """
        #log.msg("gotMessage %s -> %s" % (sender_id, frames))
        if sender_id not in self.senders:
            #log.msg("New sender %s" % sender_id)
            self.register_sender(sender_id)
        self.watchdog.report_activity(sender_id)
        if (not frames[0]   # Connection activate message from PROBE_ROUTER.
            or frames[0] == b'ping'):  # A ping message
            return  # Do not concern the clients.
        # look up what protocol is there for this sender, and pass the message on to them.
        proto = self.protocols[sender_id]
        proto.messageReceived(*frames)


class ConnectionFactory(ZmqFactory):
    def __init__(self, reactor, protocol_factory):
        log.msg("Create %s" % self.__class__.__name__)
        self.reactor = reactor
        self.protocol_factory = protocol_factory
        self.registerForShutdown()
        super().__init__()
        self.protocol_factory.startFactory()

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
        return self.connect(uri, method='bind', my_identity=my_identity)

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
        log.msg("Doing %s to %s" % endpoint)
        d = Deferred()
        reactor.callLater(0, self._create_connection, d, self, endpoint, my_identity, remote_identity)
        d.addCallback(log.msg, "connect: %s(%s) to %s(%s)" % (endpoint.type, my_identity,
                                               endpoint.address, remote_identity))
        def _report_problem(msg):
            log.msg("There's a problem %s" % msg)
            return Failure(msg)
        d.addErrback(_report_problem)
        return d

    def _create_connection(self, deferred, *args, **kwargs):
        try:
            deferred.callback(Connection(*args, **kwargs))
        except zmq.error.ZMQError as exc:
            deferred.errback(exc)

    def connection_established(self, connection, remote_identity):
        p = self.protocol_factory.buildProtocol(connection, remote_identity)
        return p

    def connection_lost(self, remote_identity):
        self.protocol_factory.connection_lost(remote_identity)