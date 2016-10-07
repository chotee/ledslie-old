from twisted.internet import reactor
from twisted.trial import unittest
from twisted.test import proto_helpers

from common.connections import ConnectionFactory
from common.proto import ProtocolFactory


class TestConnectionFactory(unittest.TestCase):
    def setUp(self):
        self.factory = ConnectionFactory(reactor, FakeProtocolFactory(FakeService()))

    # def test_connect(self):
    #     self.factory.connect("inproc://#1", my_identity="my_id", remote_identity="remote_id")


class FakeProtocolFactory(ProtocolFactory):
    pass

class FakeService(object):
    pass