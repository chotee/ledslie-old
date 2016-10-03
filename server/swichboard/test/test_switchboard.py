from twisted.trial import unittest
from twisted.test import proto_helpers

from swichboard.switchboard import SwitchboardService

class TestSwitchboardService(unittest.TestCase):

    def setUp(self):
        self.service = SwitchboardService()

    def test_forward_message(self):
        receiver = FakeRemoteClient()
        self.service.got_register("receiver", receiver)
        self.service.got_forward("receiver", "the message")
        self.assertEqual("the message", receiver.received_message)

    def test_forward_not_existing(self):
        self.service.got_forward("unknown", "the message")


class FakeRemoteClient(object):
    def send_message(self, message):
        self.received_message = message
