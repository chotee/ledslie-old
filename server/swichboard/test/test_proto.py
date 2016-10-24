from twisted.trial import unittest
from twisted.test import proto_helpers

from common.zmq_proto import ProtocolFactory, Rolodex


class TestProtocol(unittest.TestCase):
    def setUp(self):
        self.rolodex = Rolodex()
        self.factory = ProtocolFactory(self.rolodex, FakeService())
        self.connection = FakeConnection()
        self.proto = self.factory.buildProtocol(self.connection, "remote")
        self.proto.makeConnection(self.connection, remote_id="remote")

    def _test(self, message, expected):
        self.proto.messageReceived(*message)
        self.assertEqual(self.tr.value(), expected)

    # def test_send_test_message(self):
    #     return self._test(['42', 'test', 'Foo'],
    #                       ['42', 'ooF'])
    #
    # def test_register_message(self):
    #     return self._test(['21', 'register', 'somerole'],
    #                       ['21', 'registered'])

    # def test_send_unknown_message(self):
    #     self.assertRaises(AttributeError, self.proto.dataReceived, b"unknown foo\n")


class FakeConnection(object):
    pass


class FakeService(object):
    def got_register(self, client, roles):
        return "registered"

    def got_test(self, client, message):
        return message[::-1]