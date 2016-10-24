from twisted.trial import unittest
from twisted.test import proto_helpers

from swichboard.switchboard import SwitchboardService
from common.zmq_proto import Protocol, LedslieProtocol


# class TestSwitchboardService(unittest.TestCase):
#
#     def setUp(self):
#         self.service = SwitchboardService()
#         self.client = LedslieProtocol()
#
#     def test_forward_message(self):
#         client = FakeRemoteClient()
#         res = self.service.got_register(client, "role")
#         self.assertEqual(res, "Okay")
#         self.service.got_forward(client, "role", "the message")
#         self.assertEqual("the message", client.received_message)
#
#     def test_forward_not_existing(self):
#         self.service.got_forward(self.client, "send_to_role", "the message")
#
#
# class FakeRemoteClient(object):
#     def send_message(self, message):
#         self.received_message = message
