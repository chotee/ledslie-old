from twisted.python import log
from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import ServerFactory


class Protocol(LineReceiver):
    delimiter = b"\n"

    def lineReceived(self, data):
        data = data.decode("utf-8")
        try:
            op, data = data.split(None, 1)
        except ValueError:
            op = data
            data = None
        reply = self.factory.send_in(op, data, self)
        if reply:
            return self.send_message(reply)

    def send_message(self, message):
        response = "%s %s" % ("reply", message)
        return self.sendLine(response.encode('utf-8'))

    def connectionMade(self):
        log.msg("New connection")

    def connectionLost(self, reason):
        log.msg("Connection lost: %s" % reason)


class ProtocolFactory(ServerFactory):
    protocol = Protocol
    def __init__(self, service):
        self.service = service

    def send_in(self, op, contents, client):
        if op.lower() == 'register':
            return self.service.got_register(client, contents)
        elif op.lower() == 'stats':
            return self.service.got_stats(client)
        else:
            method_name = "got_%s" % op.lower()
            method = getattr(self.service, method_name)
            args = contents.split(None, 1)
            return method(client, *args)

    def startFactory(self):
        log.msg("startFactory called")

    def stopFactory(self):
        log.msg("stopFactory called")
