import os
from base64 import b64encode

from twisted.internet import protocol
from twisted.internet.defer import maybeDeferred
from twisted.internet.protocol import Factory
from twisted.python import log
from txzmq import ZmqRequestTimeoutError


class Protocol(protocol.Protocol):
    """
    I keep the state of one connection and handle the messages that might be send.
    """
    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)

    def sendMessage(self, *messageParts):
        self.transport.send(*messageParts)

    def sendRequest(self, *request_parts):
        log.msg("sendRequest %s" % repr(request_parts))
        d = self.transport.send_request(*request_parts)
        d.addCallback(self.replyReceived, request_parts)
        return d

    def messageReceived(self, *message_parts):
        log.msg("messageReceived got %s" % (repr(message_parts)))

    def replyReceived(self, reply_message_parts, request_parts):
        log.msg("replyReceived got %s -> %s" % (repr(request_parts, reply_message_parts)))

    def requestReceived(self, request_id, parts):
        log.msg("requestReceived request_id:%s, parts:%s" % (request_id,repr(parts)))
        op, args = parts[0], parts[1:]
        op_name = "req_" + op.decode()
        meth = getattr(self.factory.service, op_name)
        d = maybeDeferred(meth, self, *args)
        d.addCallback(self._reply_message, request_id)
        d.addErrback(self._reply_error, request_id)
        return d

    def _reply_message(self, message, request_id):
        self.transport.send(request_id, message)

    def _reply_error(self, failure, request_id, *args):
        self._reply_message(str(failure), request_id)


    def _cancel(self, msgId):
        """
        Cancel outstanding REQ, drop reply silently.

        @param msgId: message ID to cancel
        @type msgId: C{str}
        """
        self._requests.pop(msgId, (None, None))

    def _timeoutRequest(self, msgId):
        """
        Cancel timedout request.
        @param msgId: message ID to cancel
        @type msgId: C{str}
        """
        d, _ = self._requests.pop(msgId, (None, None))
        if not d.called:
            d.errback(ZmqRequestTimeoutError(msgId))

class LedslieProtocol(Protocol):
    def __init__(self):
        super(LedslieProtocol, self).__init__()


class ProtocolFactory(Factory):
    protocol = LedslieProtocol

    def __init__(self, rolodex, service):
        """
        I create protocols
        :param rolodex: Service that keeps track of connected guards
        :type rolodex: Rolodex
        :param service: The service providing highlevel functionality
        :type service: object
        """
        super(ProtocolFactory, self).__init__()
        self.rolodex = rolodex
        self.service = service

    def buildProtocol(self, connection, remote_id):
        try:
            proto = self.rolodex.get_proto(remote_id=remote_id)
        except KeyError:
            proto = super(ProtocolFactory, self).buildProtocol(remote_id)
            self.rolodex.add_proto(proto, remote_id)
        transport = ZmqTransport(connection, remote_id)
        proto.makeConnection(transport)
        return proto

    def connection_lost(self, remote_id):
        self.rolodex.get_proto(remote_id).connectionLost("disconnected")

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")


class Rolodex(object):
    def __init__(self):
        self.remotes = {}

    def get_proto(self, remote_id):
        return self.remotes[remote_id]

    def add_proto(self, proto, remote_id):
        self.remotes[remote_id] = proto

    def all(self):
        return self.remotes.values()


class ZmqTransport(object):
    def __init__(self, connection, remote_id):
        """
        Small helper that keeps track of the remote_id
        :param connection: The connection to the peer
        :type connection: Connection
        :param remote_id: ID of the remote connection.
        :type remote_id: str
        """
        self.connection = connection
        self.remote_id = remote_id

    def send(self, request_id, *message_parts):
        byte_frames = self._convert_to_bytes(message_parts)
        self.connection.sendMultipart(self.remote_id, [request_id] + list(byte_frames))

    def send_request(self, *message_parts):
        return self.connection.sendRequest(self.remote_id, *[p.encode() for p in message_parts])

    def _convert_to_bytes(self, data_frames):
        byte_frames = []
        for frame in data_frames:
            try:
                byte_frames.append(frame.encode())
            except AttributeError as exc:
                byte_frames.append(frame)
        return byte_frames