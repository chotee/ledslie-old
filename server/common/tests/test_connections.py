from unittest import mock

from twisted.internet import reactor
from twisted.test.proto_helpers import MemoryReactor
from twisted.test.test_application import FakeReactor
from twisted.trial import unittest
from twisted.test import proto_helpers
from zmq import Socket

from common.connections import ConnectionFactory, Connection
from common.zmq_proto import ProtocolFactory


class TestConnection(unittest.TestCase):
    def setUp(self):
        # self.reactor = MemoryReactor()
        # self.service = FakeService
        # self.protocol_factory =  FakeProtocolFactory(self.service)
        self.connection_factory = mock.MagicMock(spec=ConnectionFactory)
        self.connection_factory.context = mock.MagicMock(spec=ConnectionFactory)
        self.connection_factory.connections = set()
        self.socket = mock.MagicMock(spec=Socket)
        # self.socket = FakeSocket()

    @mock.patch("txzmq.connection.Socket")
    def test_test(self, socket):
        # listening_connection = self.connection_factory.listen("fake://", "local_test", socket=self.socket)
        #
        self.conn = Connection(self.connection_factory, socket=self.socket)
        self.assertTrue(True)


class TestConnectionFactory(unittest.TestCase):
    def setUp(self):
        self.reactor = mock.MagicMock()
        self.protocol_factory = mock.MagicMock()
        self.factory = ConnectionFactory(self.reactor, self.protocol_factory)

    def test_test(self):
        self.assertTrue(True)

    # def test_connect(self):
    #     self.factory.connect("inproc://#1", my_identity="my_id", remote_identity="remote_id")


# class FakeSocket(object):
#     def set(self, name, value):
#         pass
#
# class FakeConnectionFactory(ConnectionFactory):
#     pass
#
# class FakeProtocolFactory(ProtocolFactory):
#     pass
#
# class FakeService(object):
#     pass