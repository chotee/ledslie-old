from twisted.internet import protocol
from twisted.internet.defer import maybeDeferred
from twisted.internet.protocol import Factory
from twisted.python import log


class Protocol(protocol.Protocol):
    """
    I keep the state of one connection and handle the messages that might be send.
    """
    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)

    def messageReceived(self, *frames):
        request_id, op, args = frames[0], frames[1], frames[2:]
        log.msg("Just got %s" % (repr(frames)))
        op_name = "got_" + op
        meth = getattr(self.factory.service, op_name)
        d = maybeDeferred(meth, self, *args)
        d.addCallback(self._reply_message, request_id)
        d.addErrback(self._reply_error, request_id)
        return d

    def _reply_message(self, message, request_id):
        self.transport.send(request_id, message)

    def _reply_error(self, message, request_id, *args):
        self._reply_message("error", request_id)


class LichtkrantProtocol(Protocol):
    def __init__(self):
        super(LichtkrantProtocol, self).__init__()
        self.role = None


class ProtocolFactory(Factory):
    protocol = LichtkrantProtocol

    def __init__(self, service):
        super(ProtocolFactory, self).__init__()
        self.service = service
        self.peers = {}

    def buildProtocol(self, connection, remote_id):
        if remote_id not in self.peers:
            proto = super(ProtocolFactory, self).buildProtocol(remote_id)
            self.peers[remote_id] = proto
        else:
            proto = self.peers[remote_id]
        transport = ZmqTransport(connection, remote_id)
        proto.makeConnection(transport)
        return proto

    def connection_lost(self, addr):
        self.peers[addr].connectionLost("disconnected")

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")


class ZmqTransport(object):
    def __init__(self, connection, remote_id):
        self.connection = connection
        self.remote_id = remote_id

    def send(self, request_id, *dataframes):
        self.connection.sendMultipart(self.remote_id, [request_id] + dataframes)