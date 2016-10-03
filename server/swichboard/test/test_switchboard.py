from twisted.trial import unittest
from twisted.test import proto_helpers

from swichboard.switchboard import SwitchboardService

class TestSwitchboardService(unittest.TestCase):

    def setUp(self):
        self.service = SwitchboardService()

    def test_forward_message(self):
        receiver = FakeRemoteClient()
        self.service.register_client("receiver", receiver)
        self.service.forward_message("receiver", "the message")
        self.assertEqual("the message", receiver.received_message)

    def test_forward_not_existing(self):
        self.service.forward_message("unknown", "the message")


class FakeRemoteClient(object):
    def send_message(self, message):
        self.received_message = message