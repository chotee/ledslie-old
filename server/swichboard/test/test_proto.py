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
        self.proto.dataReceived(message+b"\r\n")
        self.assertEqual(self.tr.value(), expected+b"\r\n")

    def test_send_test_message(self):
        return self._test(b"test foo", b"reply oof")

    def test_send_unknown_message(self):
        self.assertRaises(AttributeError, self.proto.dataReceived, b"unknown foo"+b"\r\n")

class FakeService(object):
    def got_test(self, message):
        return message[::-1]