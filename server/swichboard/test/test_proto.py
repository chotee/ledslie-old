from twisted.trial import unittest
from twisted.test import proto_helpers

from swichboard.proto import ProtocolFactory


class TestProtocol(unittest.TestCase):
    def setUp(self):
        factory = ProtocolFactory(FakeService())
        self.proto = factory.buildProtocol(('127.0.0.1', 0))
        self.tr = proto_helpers.StringTransport()
        self.proto.makeConnection(self.tr)

    def _test(self, message, expected):
        self.proto.dataReceived(message)
        self.assertEqual(self.tr.value(), expected)

    def test_send_test_message(self):
        return self._test(b"test foo\n", b"reply oof\n")

    def test_register_message(self):
        return self._test(b"register foo\n", b"reply "+str(self.proto).encode()+b"\n")

    def test_send_unknown_message(self):
        self.assertRaises(AttributeError, self.proto.dataReceived, b"unknown foo\n")

class FakeService(object):
    def got_register(self, roles, client):
        return client

    def got_test(self, message):
        return message[::-1]