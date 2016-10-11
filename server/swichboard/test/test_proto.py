from twisted.trial import unittest
from twisted.test import proto_helpers

from common.zmq_proto import LedslieProtocol, ZmqTransport, ProtocolFactory


class TestProtocol(unittest.TestCase):
    def setUp(self):
        factory = ProtocolFactory(FakeService())
        connection = FakeConnection()
        self.proto = factory.buildProtocol(connection, "remote")
        self.tr = FakeZmqTransport(connection, "remote")
        self.proto.makeConnection(self.tr)

    def _test(self, message, expected):
        self.proto.messageReceived(*message)
        self.assertEqual(self.tr.value(), expected)

    def test_send_test_message(self):
        return self._test(['42', 'test', 'Foo'],
                          ['42', 'ooF'])

    def test_register_message(self):
        return self._test(['21', 'register', 'somerole'],
                          ['21', 'registered'])

    # def test_send_unknown_message(self):
    #     self.assertRaises(AttributeError, self.proto.dataReceived, b"unknown foo\n")


class FakeZmqTransport(ZmqTransport):
    def __init__(self, connection, remote_id):
        super().__init__(connection, remote_id)
        self._value = None

    def send(self, request_id, *message_parts):
        self._value = [request_id] + list(message_parts)

    def value(self):
        return self._value


class FakeConnection(object):
    pass


class FakeService(object):
    def got_register(self, client, roles):
        return "registered"

    def got_test(self, client, message):
        return message[::-1]