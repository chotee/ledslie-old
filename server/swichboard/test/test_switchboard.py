from twisted.trial import unittest
from twisted.test import proto_helpers

from swichboard.switchboard import SwitchboardFactory


class CalculationTestCase(unittest.TestCase):

    def setUp(self):
        factory = SwitchboardFactory()
        self.proto = factory.buildProtocol(('127.0.0.1', 0))
        self.tr = proto_helpers.StringTransport()
        self.proto.makeConnection(self.tr)

    def _test(self, role, data, expected):
        self.proto.dataReceived('%s %s\r\n' % (role, data))
        self.assertEqual(self.tr.value().decode("utf-8"), expected)


    def test_sendmsg(self):
        return self._test('lala', 'foo', 'oof')