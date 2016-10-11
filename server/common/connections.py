import os
from base64 import b64encode

import zmq
from twisted.internet import defer
from twisted.internet import reactor
from twisted.internet.defer import Deferred
from twisted.python import log
from twisted.python.failure import Failure
from txzmq import ZmqRouterConnection, ZmqFactory, ZmqEndpoint
from zmq import constants as zmq_constants

from common.watchdog import ConnectionWatchdog

def MessageID():
    return b64encode(os.urandom(4))[:6]


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
        self._requests = {}
        self._routingInfo = {}
        if self.connecting:
            assert remote_identity, "For connecting, a remote identity is required."
            self.register_remote(remote_identity)

    def register_remote(self, remote_id):
        """
        I register a sender with the system so that the connection will be monitored.
        :param remote_id: Identity of the sender to monitor
        :type remote_id: str
        """
        self.senders.add(remote_id)
        self.watchdog.start_monitor(remote_id)

    def sender_established(self, remote_id):
        """
        I get called when we can confirm a connection with a sender has been established.
        :param remote_id: The ID of the sender that we have established
        :type remote_id: str
        """
        log.msg("established connection to %s." % remote_id)
        protocol = self.factory.connection_established(self, remote_id)
        self.protocols[remote_id] = protocol

    def sender_disconnected(self, sender_id):
        """
        I get called when we have not received any traffic from a sender.
        :param sender_id: The ID of the sender that we have lost
        :type sender_id: str
        """
        log.msg("lost connection to %s" % sender_id)
        self.factory.connection_lost(sender_id)

    def gotMessage(self, remote_id, *frames):
        """
        I get called when a message is received on the socket. If the message is a ping, I inform the watchdog.
        If it's any other, I pass it on to the object maintaining state with the recipient.
        :param remote_id: The Id of the sender having send the connection
        :type remote_id: str
        :param frames: All the frames of the message.
        :type frames: tuple
        """
        log.msg("gotMessage remote_id:%s frames:%s" % (remote_id, repr(frames)))
        if remote_id not in self.senders:
            #log.msg("New sender %s" % sender_id)
            self.register_remote(remote_id)
        self.watchdog.report_activity(remote_id)
        if (not frames[0]   # Connection activate message from PROBE_ROUTER.
            or frames[0] == b'ping'):  # A ping message
            return  # Done handling the ping.

        proto = self.protocols[remote_id]
        if len(frames) > 2 and frames[1] == b'':  # This looks like a request message
            msgId, msg = frames[0], frames[2:]
            if msgId in self._requests:
                self.requestReplyReceived(msgId, msg)
            else:
                proto.requestReceived(*self.requestReceived(remote_id, msgId, msg))
                return
        # look up what protocol is there for this sender, and pass the message on to them.

        proto.messageReceived(*frames)

    def sendRequest(self, remote_id, *messageParts, **kwargs):
        """
        Send request and deliver response back when available.

        :param messageParts: message data
        :type messageParts: tuple
        :param timeout: as keyword argument, timeout on request
        :type timeout: float
        :return: Deferred that will fire when response comes back
        """
        message_id = MessageID()
        d = defer.Deferred(canceller=lambda _: self._cancel(message_id))

        timeout = kwargs.pop('timeout', None)
        # if timeout is None:
        #     timeout = self.defaultRequestTimeout
        assert len(kwargs) == 0, "Unsupported keyword argument"

        canceller = None
        if timeout is not None:
            canceller = reactor.callLater(timeout, self._timeoutRequest,
                                          message_id)

        self._requests[message_id] = (d, canceller)
        self.send([remote_id, message_id, b''] + list(messageParts))
        return d

    def requestReplyReceived(self, msgId, msg):
        """
        Called on incoming request reply from ZeroMQ.

        Dispatches message to back to the requestor.

        :param message: message data
        """
        log.msg("requestReplyReceived %s -> %s" % (msgId, msg))
        d, canceller = self._requests.pop(msgId, (None, None))

        if canceller is not None and canceller.active():
            canceller.cancel()

        if d is None:
            # reply came for timed out or cancelled request, drop it silently
            log.msg("Received expired reply %s" % msgId)
            return

        d.callback(msg)

    def requestReceived(self, remote_id, msgId, msg):
        """
        Called on incoming request message from ZeroMQ.

        :param message: message data
        """
        # i = message.index(b'')
        # assert i > 0
        # (routingInfo, msgId, payload) = (
        #     message[:i - 1], message[i - 1], message[i + 1:])
        msgParts = msg[0:]
        # self._routingInfo[msgId] = remote_id
        return msgId, msgParts


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